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
#include <FL/Fl_Toggle_Button.H>
#include <sstream>
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

Card top_card;

void printTop(char (&message)[PACKET_SIZE]){
  int8_t value;
  char color;
  memcpy(&value, message+ACTION_B_COUNT, sizeof(int8_t));
  memcpy(&color, message+ACTION_B_COUNT+sizeof(int8_t), sizeof(char));
  top_card.value = value;
  top_card.color = color;
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
Card* target;
char color = 'r'; //default to red

void button_cb(Fl_Widget *w, void *z){
  target = (Card*)z;
  cout << "button pressed!\n" << (int)((*(Card*)z).value) << endl;
  responded = false;
}

void send_draw(Fl_Widget *w, void *z){
  *((char*)z) = 'd';
  responded = false;
}

Fl_Toggle_Button *color_swap[4];
void set_color(Fl_Widget *w, void *color_set){
  color = *(char*)color_set;
  for(int i = 0; i < 4; i++){
    color_swap[i]->value(0);
  }
  ((Fl_Toggle_Button*)w)->value(1);
}

string applySym(string input){
  if(input == "-1")
    return "S";
  if(input == "-2")
    return "+2";
  if(input == "-3")
    return "R";
  if(input == "-4")
    return "W";
  if(input == "-5")
    return "+4";
  if(input == "10")
    return "W";
  return input;
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
  FL_NORMAL_SIZE = 24;
  char *host = argv[1];
  int socket = lookup_and_connect(host, argv[2]);
  unsigned char key[256] = {0};
  unsigned char aes_key[32];
  RAND_bytes(aes_key, sizeof(aes_key));
  RSA* rsa_pub = load_public_key("public.pem");
  int enc_key_len = RSA_public_encrypt(sizeof(aes_key), aes_key, key, rsa_pub, RSA_PKCS1_OAEP_PADDING);
  send(socket, key, enc_key_len, 0);
  cout << socket << endl;
  bool game_running = true;

  char play;
  int pos;
  bool sent_status;
  vector<Card> hand;
  char msg1[PACKET_SIZE] = {'\0'};
  char msg2[PACKET_SIZE] = {'\0'};
  char msg3[PACKET_SIZE] = {'\0'};
  char msg4[PACKET_SIZE] = {'\0'};
  char col_opt[4] = {'r','b','y','g'};
  Fl_Window *window = new Fl_Window(1500,950);
  window->fullscreen();
  Fl_Button *drawer = new Fl_Button(600, 40, 200, 40, "Press to Draw!");
  drawer->callback(send_draw, (void*)&play);
  Fl_Toggle_Button *red = new Fl_Toggle_Button(200, 40, 90, 40, "Red");
  color_swap[0] = red;
  red->color(FL_RED);
  red->callback(set_color, (void*)&col_opt[0]);
  Fl_Toggle_Button *blue = new Fl_Toggle_Button(400, 40, 90, 40, "Blue");
  color_swap[1] = blue;
  blue->color(FL_BLUE);
  blue->callback(set_color, (void*)&col_opt[1]);
  Fl_Toggle_Button *yellow = new Fl_Toggle_Button(960, 40, 90, 40, "Yellow");
  color_swap[2] = yellow;
  yellow->color(FL_YELLOW);
  yellow->callback(set_color, (void*)&col_opt[2]);
  Fl_Toggle_Button *green = new Fl_Toggle_Button(1160, 40, 90, 40, "Green");
  color_swap[3] = green;
  green->color(FL_GREEN);
  green->callback(set_color, (void*)&col_opt[3]);
  red->set();
  Fl_Box *display_box = new Fl_Box(1190,200,280,360);
  display_box->box(FL_UP_BOX);
  display_box->labelsize(100);
  display_box->labelfont(FL_BOLD + FL_HELVETICA);
  window->end();
  window->show();
  //swap to Fl_Button later
  Fl_Button *display[12][9] = {NULL}; //12 colums, 9 rows
  while(game_running && window->shown()){
    responded = true;
    top_card.value = -20;
    top_card.color = 'e';
    play = 'E';
    hand.clear();
    pos = -1;
    sent_status = false;
    msg1[0] = '\0';
    msg2[0] = '\0';
    msg3[0] = '\0';
    msg4[0] = '\0';
    int rec = recv(socket, msg1, GAME_INFO_SIZE, 0);
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg2, 224, 0);
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg3, 73, 0);
    game_running = (game_running && checkRec(rec, socket));
    rec = recv(socket, msg4, TOP_INFO_SIZE, 0);
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
        str = applySym(str);
        const char* convert = str.c_str();
        if(display[col][row] != NULL){
          display[col][row]->hide();
          display[col][row] = NULL;
        }
        Fl_Button *box = new Fl_Button(20+(col*90),90+(row*100),80,90);
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
        box->redraw();
        box->labelfont(FL_HELVETICA_BOLD);
        box->labelsize(40);
        box->labelcolor(FL_BLACK);
        display[col][row] = box;
      }
      stringstream ss;
      ss << (int)top_card.value;
      string str = ss.str();
      str = applySym(str);
      const char* convert = str.c_str();
      if(top_card.color == 'r')
        display_box->color(FL_RED);
      else if(top_card.color == 'b')
        display_box->color(FL_BLUE);
      else if(top_card.color == 'g')
        display_box->color(FL_GREEN);
      else if(top_card.color == 'y')
        display_box->color(FL_YELLOW);
      display_box->label(convert);
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

      pos = (int8_t)distance(hand.data(), target);
      cout << "What would you like to do?\nd to draw, or p to play or w to play wild card\n";
      if(play != 'd'){
        if(hand.at(pos).color == 'w')
          play = 'w';
        else
          play = 'p';
      }
      switch(play){
        case('d'):
          sent_status = sendDraw(socket);
          break;
        case('p'):
          cout << pos << "What card?\n";
          //cin >> pos;
          sent_status = sendCard(socket, pos);
          break;
        case('w'):
          cout << "What card?\n";
          //cin >> pos;
          cout << "What color (r,g,b, or y)\n";
          //cin >> color;
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
