#include "cards.h"
#include <vector>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/err.h>

using namespace std;

#define PACKET_SIZE 2000

int lookup_and_connect(const char *host, const char *service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Translate host name into peer's IP address */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ((s = getaddrinfo(host, service, &hints, &result)) != 0) {
    fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }

  /* Iterate through the address list and try to connect */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
      continue;
    }
    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }
    close(s);
  }

  if (rp == NULL) {
    perror("stream-talk-client: connect");
    return -1;
  }

  freeaddrinfo(result);
  return s;
}

void printCard(char color, int value, int position){
  //color first
  if(position != -1)
    cout << position << ": ";
  else
    cout << "Card to match: ";
  if(color != 'w'){
    switch(color){
      case 'r':
        cout << "Red";
        break;
      case 'b':
        cout << "Blue";
        break;
      case 'g':
        cout << "Green";
        break;
      case 'y':
        cout << "Yellow";
        break;
    }
    cout << ": ";
    switch(value){
      case -1:
        cout << "Skip";
        break;
      case -2:
        cout << "Draw 2";
        break;
      case -3:
        cout << "Reverse";
        break;
      default:
        cout << value;
        break;
    }
    cout << endl;
  }
  else{
    cout << "Wild";
    if(value == -5)
      cout << " Draw 4";
    cout << endl;
  }
}

bool sendDraw(const int socket){
  char packet[DRAW_PACKET_SIZE];
  uint8_t action = DRAW_COMMAND;
  memcpy(packet, &action, ACTION_B_COUNT); // copy the action command
  int bytes_sent = 0;
  int packet_size = sizeof(packet);
  int current_send = 0;
  while(bytes_sent != packet_size){
    current_send = send(socket,packet+bytes_sent,packet_size-bytes_sent,0);
    if (current_send == -1){
      return current_send;
    }
    bytes_sent += current_send;
  }
  return (bytes_sent == DRAW_PACKET_SIZE);
}

bool sendCard(const int socket, int8_t card){
  char packet[PLAY_PACKET_SIZE];
  uint8_t action = PLAY_COMMAND;
  memcpy(packet, &action, ACTION_B_COUNT);
  memcpy(packet+ACTION_B_COUNT, &card, sizeof(int8_t));
  int bytes_sent = 0;
  int packet_size = sizeof(packet);
  int current_send = 0;
  while(bytes_sent != packet_size){
    current_send = send(socket,packet+bytes_sent,packet_size-bytes_sent,0);
    if (current_send == -1){
      return current_send;
    }
    bytes_sent += current_send;
  }
  return (bytes_sent == PLAY_PACKET_SIZE);
}

//the int here is the card's position in the hand, not the value, since uno deck is <127, won't go over size
bool sendWildCard(const int socket, int8_t card, char color){
  char packet[PLAY_WILD_SIZE];
  uint8_t action = PLAY_WILD_COMMAND;
  memcpy(packet, &action, ACTION_B_COUNT);
  memcpy(packet+ACTION_B_COUNT, &card, sizeof(int8_t));
  memcpy(packet+ACTION_B_COUNT+sizeof(int8_t), &color, sizeof(char));
  int bytes_sent = 0;
  int packet_size = sizeof(packet);
  int current_send = 0;
  while(bytes_sent != packet_size){
    current_send = send(socket,packet+bytes_sent,packet_size-bytes_sent,0);
    if (current_send == -1){
      return current_send;
    }
    bytes_sent += current_send;
  }
  return (bytes_sent == PLAY_WILD_SIZE);
}

void printInfo(char (&message)[PACKET_SIZE]){
  int size = sizeof(int);
  int id;
  int p_count;
  int next;
  memcpy(&id, message + ACTION_B_COUNT, size);
  memcpy(&next, message + ACTION_B_COUNT + size, size);
  memcpy(&p_count, message + ACTION_B_COUNT + size + size, size);
  id = ntohl(id);
  p_count = ntohl(p_count);
  next = ntohl(next);
  cout << "You are: " << id << " facing " << (p_count-1) << " players, and player number " << next << " is next\n";
}

unsigned char aes_key[32];

void printHand(char (&message)[PACKET_SIZE]){
  uint8_t hand_size = 0;
  memcpy(&hand_size, message + ACTION_B_COUNT, sizeof(uint8_t));
  if(hand_size == 0){
    cout << "Hand empty you won!";
    return;
  }
  else{
    int len = 0;
    unsigned char cards[216];
    unsigned char cut[224];
    memcpy(cut, message + 18, 224);
    unsigned char iv[16];
    memcpy(iv, message + 2, 16);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aes_key, iv);
    EVP_DecryptUpdate(ctx, cards, &len, cut, 224);
    EVP_DecryptFinal_ex(ctx, cards + len, &len);
    EVP_CIPHER_CTX_free(ctx);
    
    char cur_color = 'e';
    int8_t cur_value = 0;
    int offset = sizeof(char) + sizeof(int8_t);
    for(uint8_t i = 0; i < hand_size; i++){
      memcpy(&cur_color, cards + (i * offset), sizeof(char));
      memcpy(&cur_value, cards + sizeof(char) + (i * offset), sizeof(int8_t));
      printCard(cur_color, (int)cur_value, i);
    }
  }
}

void printOppInfo(char (&message)[PACKET_SIZE]){
  int id = 0;
  int hand_count = 0;
  for(int i = 0; i < 9; i++){ //10 player max
    memcpy(&id, message + ACTION_B_COUNT + (i*sizeof(int)*2),  sizeof(int));
    memcpy(&hand_count, message + ACTION_B_COUNT + (i*sizeof(int)*2) + sizeof(int),  sizeof(int));
    int id_h = ntohl(id);
    int hand_count_h = ntohl(hand_count);
    if(hand_count_h > 0)
      cout << "Oponnent " << id_h << " has " << hand_count_h << " card(s) in hand)\n";
    id = 0;
    hand_count = 0;
  }
}

void printTop(char (&message)[PACKET_SIZE]){
  int8_t value;
  char color;
  memcpy(&value, message+ACTION_B_COUNT, sizeof(int8_t));
  memcpy(&color, message+ACTION_B_COUNT+sizeof(int8_t), sizeof(char));
  printCard(color, value, -1);
}

void processMsg(char (&recvMsg)[PACKET_SIZE]){
  uint8_t flag;
  memcpy(&flag, recvMsg, sizeof(flag));
  if(flag == GAME_INFO_FLAG){
    printInfo(recvMsg);
    return;
  }
  if(flag == HAND_INFO_FLAG){
    printHand(recvMsg);
    return;
  }
  if(flag == OPPONENT_INFO){
    printOppInfo(recvMsg);
    return;
  }
  if(flag == TOP_INFO_FLAG){
    printTop(recvMsg);
    return;
  }
}

bool recv_all(int socket, char (&recvMsg)[PACKET_SIZE], int expect){
  int total_rec = 0;
  while(total_rec < expect){
    int bytes = recv(socket, recvMsg, expect - total_rec, 0);
    if(bytes <= 0)
      return false;
    total_rec += bytes;
  }
  return true;
}

RSA* load_public_key(const char* filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    perror("Failed to open public key file");
    return nullptr;
  }
  RSA* rsa = PEM_read_RSA_PUBKEY(fp, nullptr, nullptr, nullptr);
  fclose(fp);
  if (!rsa) {
    fprintf(stderr, "Error reading public key from file\n");
  }
  return rsa;
}

int main(int argc, char *argv[]){
  vector<Card> hand;
  char *host = argv[1];
  int socket = lookup_and_connect(host, argv[2]);
  unsigned char key[256] = {0};
  RAND_bytes(aes_key, sizeof(aes_key));
  RSA* rsa_pub = load_public_key("public.pem");
  int enc_key_len = RSA_public_encrypt(sizeof(aes_key), aes_key, key, rsa_pub, RSA_PKCS1_OAEP_PADDING);
  send(socket, key, enc_key_len, 0);
  bool game_running = true;

  char play;
  int pos;
  char color;
  bool sent_status;

  char msg1[PACKET_SIZE] = {'\0'};
  char msg2[PACKET_SIZE] = {'\0'};
  char msg3[PACKET_SIZE] = {'\0'};
  char msg4[PACKET_SIZE] = {'\0'};
  while(game_running){
    play = 'E';
    pos = -1;
    color = 'e';
    sent_status = false;
    msg1[0] = '\0';
    msg2[0] = '\0';
    msg3[0] = '\0';
    msg4[0] = '\0';
    bool rec = recv_all(socket, msg1, GAME_INFO_SIZE);
    game_running = (game_running && rec);
    rec = recv_all(socket, msg2, 242);
    game_running = (game_running && rec);
    rec = recv_all(socket, msg3, 73);
    game_running = (game_running && rec);
    rec = recv_all(socket, msg4, TOP_INFO_SIZE);
    game_running = (game_running && rec);

    if(game_running) {
      processMsg(msg1);
      processMsg(msg2);
      processMsg(msg3);
      processMsg(msg4);
      cout << "What would you like to do?\nd to draw, or p to play or w to play wild card\n";
      cin >> play;
      switch(play){
        case('d'):
          sent_status = sendDraw(socket);
          break;
        case('p'):
          cout << "What card?\n";
          cin >> pos;
          sent_status = sendCard(socket, pos);
          break;
        case('w'):
          cout << "What card?\n";
          cin >> pos;
          cout << "What color (r,g,b, or y)\n";
          cin >> color;
          sent_status = sendWildCard(socket, pos, color);
          break;
        default:
          cout << "Bad input, drawing\n";
          sent_status = sendDraw(socket);
      }
      if(sent_status){
        cout << "sent success\n";
      }
    }
  }
  cout << "game over\n";
}
