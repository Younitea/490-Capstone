#include "cards.h"
#include <vector>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <sstream>
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
    int offset = sizeof(char) + sizeof(int8_t);
    for(uint8_t i = 0; i < hand_size; i++){
      memcpy(&cur_color, message + base_offset + (i * offset), sizeof(char));
      memcpy(&cur_value, message + base_offset + sizeof(char) + (i * offset), sizeof(int8_t));
      Card cur;
      cur.value = cur_value;
      cur.color = cur_color;
      hand.push_back(cur);
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

void processMsg(char (&recvMsg)[PACKET_SIZE], vector<Card> &hand){
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
    printTop(recvMsg);
    return;
  }
}

bool responded;

void button_cb(Fl_Widget *w, void *z){
  cout << "button pressed!\n" << (int)((*(Card*)z).value) << endl;
  responded = false;
}

int main(int argc, char *argv[]){
  char *host = argv[1];
  int port = atoi(argv[2]);
  int socket = lookup_and_connect(host, argv[2]);
  cout << socket << endl;
  bool game_running = true;

  char play;
  int pos;
  char color;
  bool sent_status;
  vector<Card> hand;
  char msg1[PACKET_SIZE] = {'\0'};
  char msg2[PACKET_SIZE] = {'\0'};
  char msg3[PACKET_SIZE] = {'\0'};
  char msg4[PACKET_SIZE] = {'\0'};
  Fl_Window *window = new Fl_Window(1500,900);
  window->end();
  window->show();
  //swap to Fl_Button later
  Fl_Button *display[12][9] = {NULL}; //12 colums, 9 rows
  while(game_running && window->shown()){
    responded = true;
    play = 'E';
    hand.clear();
    pos = -1;
    color = 'e';
    sent_status = false;
    msg1[0] = '\0';
    msg2[0] = '\0';
    msg3[0] = '\0';
    msg4[0] = '\0';
    int rec = recv(socket, msg1, sizeof(msg1), 0);
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg2, sizeof(msg2), 0);
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg3, sizeof(msg3), 0);
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg4, sizeof(msg4), 0);
    game_running = (game_running && checkRec(rec, socket));
    if(game_running) {
      cout << "something recieved!\n";
      processMsg(msg1, hand);
      processMsg(msg2, hand);
      processMsg(msg3, hand);
      processMsg(msg4, hand);
      //in theory, should now have hand info

      int col = 0;
      int row = 0;
      size_t num_cards;
      window->begin();
      for(num_cards = 0; num_cards < hand.size(); num_cards++){
        col = num_cards / 9;
        row = num_cards % 9;
        stringstream ss;
        ss << (int)hand.at(num_cards).value;
        string str = ss.str();
        const char* convert = str.c_str();
        if(display[col][row] != NULL){
          display[col][row]->hide();
          display[col][row] = NULL;
        }
        Fl_Button *box = new Fl_Button(20+(col*60),40+(row*70),40,50);
        //box->box(FL_UP_BOX);
        //box->callback(card_cb, status)
        box->copy_label(convert);
        char color1 = hand.at(num_cards).color;
        box->callback(button_cb, (void*)(&hand.at(num_cards)));
        if(color1 == 'r')
          box->color(FL_RED);
        else if(color1 == 'b')
          box->color(FL_BLUE);
        else if(color1 == 'g')
          box->color(FL_GREEN);
        else if(color1 == 'y')
          box->color(FL_YELLOW);
        else
          box->color(FL_GRAY);
        display[col][row] = box;
      }
      if(display[num_cards / 9][num_cards % 9] != NULL){
        display[num_cards / 9][num_cards % 9]->hide();
        display[num_cards / 9][num_cards % 9] = NULL;
      }
      window->end();
      window->redraw();
      Fl::check();
      window->redraw();
      while(responded && Fl::wait()){}
      Fl::check();

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
