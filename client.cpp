#include "cards.h"
#include <vector>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <iostream>

#define ACTION_B_COUNT 1
#define START_COMMAND 10
#define START_PACKET_SIZE 1

using namespace std;

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

void printCard(char color, int value){
  //color first
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


void printHand(char (&message)[1205]){
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
      printCard(cur_color, (int)cur_value);
    }
  }
}

int startGame(const int socket);

int main(int argc, char *argv[]){
  vector<Card> hand;
  char *host = argv[1];
  int port = atoi(argv[2]);
  string input;
  int socket = lookup_and_connect(host, argv[2]);
  cout << socket << endl;
  bool game_running = true;

  uint8_t flag = 0;

  char recvMsg[1205] = {'\0'};

  while(game_running){
    getline(cin, input);
    if(input == "START"){
      int bytes_sent = startGame(socket);
      if (bytes_sent != START_PACKET_SIZE) {
        cerr << "Error sending join packet\n";
      }
    }
    if(input == "LISTEN"){
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
          if(flag == 4){
            printHand(recvMsg);
            recvMsg[0] = '\0';
          }
      }
    }
  }
  cout << "game over";
}

int startGame(const int socket){
  char packet[START_PACKET_SIZE];
  uint8_t action = START_COMMAND;
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
