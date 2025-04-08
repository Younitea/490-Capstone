#include "cards.h"
#include <vector>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>

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
  cout << "You are: " << id << " facing " << (p_count-1) << " players, and player number " << next << " is next\n";
}

void printHand(char (&message)[PACKET_SIZE], vector<Card> &hand){
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
    Card add;
    int offset = sizeof(char) + sizeof(int8_t);
    for(uint8_t i = 0; i < hand_size; i++){
      memcpy(&cur_color, message + base_offset + (i * offset), sizeof(char));
      memcpy(&cur_value, message + base_offset + sizeof(char) + (i * offset), sizeof(int8_t));
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

int main(int argc, char *argv[]){
  vector<Card> hand;
  char *host = argv[1];
  int port = atoi(argv[2]);
  int socket = lookup_and_connect(host, argv[2]);
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
  char rand_col = 'r'; //this should be made actually random, but will worry about it later lol

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

    rec = recv(socket, msg1, sizeof(msg1), 0);

    cout << "rec 1\n";
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg2, sizeof(msg2), 0);
    cout << "rec 2\n";
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg3, sizeof(msg3), 0);
    cout << "rec 3\n";
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg4, sizeof(msg4), 0);
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
      for(size_t i = 0; i < hand.size(); i++){
        if((hand.at(i).color == 'w') || (col == hand.at(i).color) || (val == hand.at(i).value)){
          match_found = true;
          match = i;
          cout << "Match found: ";
          printCard(hand.at(i).color, hand.at(i).value, i);
          break;
        }
      }
      if(match_found){
        if(hand.at(match).color != 'w')
          sent_status = sendCard(socket, match);
        else
          sent_status = sendWildCard(socket, match, rand_col);
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
