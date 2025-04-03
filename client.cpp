#include "cards.h"
#include <vector>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <iostream>

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

int sendDraw(const int socket){
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
  return bytes_sent;
}

int sendCard(const int socket, int8_t card){
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
  return bytes_sent;
}

//the int here is the card's position in the hand, not the value, since uno deck is <127, won't go over size
int sendWildCard(const int socket, int8_t card, char color){
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
  return bytes_sent;
}

void printInfo(char (&message)[PACKET_SIZE]){
  int size = sizeof(int);
  int id;
  int p_count;
  int next;
  memcpy(&id, message + ACTION_B_COUNT, size);
  memcpy(&p_count, message + ACTION_B_COUNT + size, size);
  memcpy(&next, message + ACTION_B_COUNT + size + size, size);
  cout << "You are: " << id << " facing " << (p_count-1) << " players, and player number " << next << " is next\n";
}

void printHand(char (&message)[PACKET_SIZE]){
  uint8_t hand_size = 0;
  memcpy(&hand_size, message + ACTION_B_COUNT, sizeof(uint8_t));
  if(hand_size == 0){
    cout << "Hand empty you won!";
    return;
  }
  else{
    int base_offset = ACTION_B_COUNT + sizeof(uint8_t);
    char cur_color = 'e';
    int8_t cur_value = 0;
    int offset = sizeof(char) + sizeof(int8_t);
    for(uint8_t i = 0; i < hand_size; i++){
      memcpy(&cur_color, message + base_offset + (i * offset), sizeof(char));
      memcpy(&cur_value, message + base_offset + sizeof(char) + (i * offset), sizeof(int8_t));
      printCard(cur_color, (int)cur_value, i);
    }
  }
}

void printTop(char (&message)[PACKET_SIZE]){
  int8_t value;
  char color;
  memcpy(&value, message+ACTION_B_COUNT, sizeof(int8_t));
  memcpy(&color, message+ACTION_B_COUNT+sizeof(int8_t), sizeof(char));
  printCard(color, value, -1);
}

int main(int argc, char *argv[]){
  vector<Card> hand;
  char *host = argv[1];
  int port = atoi(argv[2]);
  int socket = lookup_and_connect(host, argv[2]);
  cout << socket << endl;
  bool game_running = true;

  uint8_t flag = 0;

  char recvMsg[PACKET_SIZE] = {'\0'};

  while(game_running){
    int rec = recv(socket, recvMsg, sizeof(recvMsg), 0);
    if (rec < 0) {
      perror("ERROR in recv() call");
      close(socket);
      exit(1);
    } else if (rec == 0) {
      cout << "Socket " << socket << " closed" << endl;
      game_running = false;
      continue;
    }
    else {
      cout << "something recieved!\n";
      memcpy(&flag, recvMsg, sizeof(flag));
      if(flag == GAME_INFO_FLAG){
        printInfo(recvMsg);
        recvMsg[0] = '\0';
        continue;
      }
      if(flag == HAND_INFO_FLAG){
        printHand(recvMsg);
        recvMsg[0] = '\0';
        continue;
      }
      if(flag == TOP_INFO_FLAG){
        printTop(recvMsg);
        recvMsg[0] = '\0';
        int input;
        cin >> input;
        if(PLAY_PACKET_SIZE != sendCard(socket, input))
          cout << "Bad send\n";
        continue;
      }
    }
  }
  cout << "game over";
}
