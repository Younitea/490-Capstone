#include "cards.h"
#include <vector>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/err.h>

using namespace std;

#define PACKET_SIZE 250

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

void printHand(char (&message)[PACKET_SIZE], vector<Card> &hand){
  uint8_t hand_size = 0;
  memcpy(&hand_size, message + ACTION_B_COUNT, sizeof(uint8_t));
  if(hand_size == 0){
    cout << "Hand empty you won!";
    return;
  }
  else{
    int len = 0;
    cout << (int) hand_size << endl;
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


    printf("msg2[3] = 0x%02x\n", (unsigned char)message[3]);
    printf("msg2[19] = 0x%02x\n", (unsigned char)message[19]);
    char cur_color = 'e';
    int8_t cur_value = 0;
    Card add;
    int offset = sizeof(char) + sizeof(int8_t);
    for(uint8_t i = 0; i < hand_size; i++){
      memcpy(&cur_color, cards + (i * offset), sizeof(char));
      memcpy(&cur_value, cards + sizeof(char) + (i * offset), sizeof(int8_t));
      printCard(cur_color, (int)cur_value, i);
      add.color = cur_color;
      add.value = cur_value;
      hand.push_back(add);
    }
  }
}

void printTop(char (&message)[PACKET_SIZE], Card &top){
  int8_t value;
  char color;
  memcpy(&value, message+ACTION_B_COUNT, sizeof(int8_t));
  memcpy(&color, message+ACTION_B_COUNT+sizeof(int8_t), sizeof(char));
  printCard(color, value, -1);
  top.value = value;
  top.color = color;
}

bool checkRec(int rec, int socket){
  if(rec < 0){
    perror("ERROR in recv() call");
    close(socket);
    exit(1);
  }
  else if(rec == 0){
    cout << "Socket " << socket << " closed\n";
    return false;
  }
  else
    return true;
}

void processMsg(char (&recvMsg)[PACKET_SIZE], vector<Card> &hand, Card &top){
  uint8_t flag;
  memcpy(&flag, recvMsg, sizeof(flag));
  if(flag == GAME_INFO_FLAG){
    printInfo(recvMsg);
    return;
  }
  if(flag == HAND_INFO_FLAG){
    printHand(recvMsg, hand);
    return;
  }
  if(flag == TOP_INFO_FLAG){
    printTop(recvMsg, top);
    return;
  }
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

char pickColor(vector<Card> &hand){
  int colors[4] = {0};
  for(size_t i = 0; i < hand.size(); i++){
    switch(hand.at(i).color){
      case 'r':
        colors[0]++;
        break;
      case 'b':
        colors[1]++;
        break;
      case 'y':
        colors[2]++;
        break;
      case 'g':
        colors[3]++;
        break;
    }
  }
  int max = 0;
  if(colors[1] > max)
    max = 1;
  if(colors[2] > max)
    max = 2;
  if(colors[3] > max)
    max = 3;
  switch(max){
    case 0:
      return 'r';
    case 1:
      return 'b';
    case 2:
      return 'y';
    case 3:
      return 'g';
  }
  return 'r';
}

int find_best(vector<int> &matches, vector<Card> &hand, char col, int8_t val){
  //find a special card with matching color first
  for(size_t i = 0; i < matches.size(); i++){
    Card cur = hand.at(matches.at(i));
    if(cur.color == col && cur.value < 0)
      return matches.at(i);
  }
  //check for regular card with matching color
  for(size_t i = 0; i < matches.size(); i++){
    Card cur = hand.at(matches.at(i));
    if(cur.color == col && cur.value > -1)
      return matches.at(i);
  }
  //check for matching value next
  for(size_t i = 0; i < matches.size(); i++){
    Card cur = hand.at(matches.at(i));
    if(cur.value == val)
      return matches.at(i);
  }
  //all else fails send a wild card (+4 first) and then (non)
  for(size_t i = 0; i < matches.size(); i++){
    Card cur = hand.at(matches.at(i));
    if(cur.color == 'w' && cur.value == -5)
      return matches.at(i);
  }
  for(size_t i = 0; i < matches.size(); i++){
    Card cur = hand.at(matches.at(i));
    if(cur.color == 'w' && cur.value == -4)
      return matches.at(i);
  }
  //then fall back to the first match if nothing else (shouldn't be needed, so including a error message)
  cout << "Matching failed after match found ;(\n";
  return (matches.at(0));
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
  cout << socket << endl;
  bool game_running = true;

  bool sent_status;
  Card top;

  char msg1[PACKET_SIZE] = {'\0'};
  char msg2[PACKET_SIZE] = {'\0'};
  char msg3[PACKET_SIZE] = {'\0'};
  char msg4[PACKET_SIZE] = {'\0'};
  char col;
  int8_t val;
  bool match_found;
  int rec;
  int match;
  char wild_col = 'r'; //Default to red

  while(game_running){
    cout << "top of the loop\n";
    rec = 0;
    match = -1;
    top.value = -1;
    top.color = 'E';
    cout << "values set ";
    hand.clear();
    sent_status = false;
    cout << "hand cleared ";
    msg1[0] = '\0';
    msg2[0] = '\0';
    msg3[0] = '\0';
    msg4[0] = '\0';
    cout << "msgs set\nprior to\n";

    rec = recv(socket, msg1, GAME_INFO_SIZE, 0);
    cout << "rec 1\n";
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg2, 242, 0);
    cout << "rec 2\n";
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg3, 73, 0);
    cout << "rec 3\n";
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg4, TOP_INFO_SIZE, 0);
    cout << "rec 4\n";
    game_running = (game_running && checkRec(rec, socket));

    if(game_running) {
      cout << "something recieved!\n";
      processMsg(msg1, hand, top);
      processMsg(msg2, hand, top);
      processMsg(msg3, hand, top);
      processMsg(msg4, hand, top);
      col = top.color;
      val = top.value;
      match_found = false;
      vector<int> matches;
      for(size_t i = 0; i < hand.size(); i++){
        if((hand.at(i).color == 'w') || (col == hand.at(i).color) || (val == hand.at(i).value)){
          match_found = true;
          match = i;
          cout << "Match found: ";
          printCard(hand.at(i).color, hand.at(i).value, i);
          matches.push_back(i);
        }
      }
      if(match_found){
        match = find_best(matches, hand, col, val);
        if(hand.at(match).color != 'w')
          sent_status = sendCard(socket, match);
        else{
          wild_col = pickColor(hand);
          sent_status = sendWildCard(socket, match, wild_col);
        }
      }
      else{
        sent_status = sendDraw(socket);
      }
      if(sent_status){
        cout << "sent success\n";
      }
      else
        cerr << "Send error\n";
      cout << "bot of the loop and game running is: " << game_running << endl;
    }
  }
  cout << "game over\n";
}
