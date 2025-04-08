#include "cards.h"
#include "server_uno.h"
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <chrono>
#include <thread>

#define MAX_PENDING 5
#define ACTION_B_COUNT 1

int find_max_fd(const fd_set *fs) {
  int ret = 0;
  for(int i = FD_SETSIZE; i>=0 && ret==0; --i){
    if( FD_ISSET(i, fs) ){
      ret = i;
    }
  }
  return ret;
}

int bind_and_listen( const char *service ){
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;
  memset( &hints, 0, sizeof( struct addrinfo ));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  if((s = getaddrinfo(NULL, service, &hints, &result)) != 0){
    fprintf(stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }
  for( rp = result; rp != NULL; rp = rp->ai_next){
    if(( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1){
      continue;
    }

    if( !bind(s, rp->ai_addr, rp->ai_addrlen) ){
      break;
    }
    close( s );
  }
  if( rp == NULL){
    perror("stream-talk-server: bind");
    return -1;
  }
  if( listen( s, MAX_PENDING ) == -1){
    perror("stream-talk-server: listen");
    close( s );
    return -1;
  }
  freeaddrinfo( result );

  return s;
}


int calcNextPlayer(int count, int current, bool clock){
  if(clock){
    current++;
    if(current >= count)
      current = 0;
  }
  else{
    current--;
    if(current < 0)
      current = count - 1;
  }
  return current;
}

bool checkWildInput(char color){
  bool valid = false;
  if(color == 'r' || color == 'g' || color == 'b' || color == 'y')
    valid = true;
  return valid;
}

bool sendGameInfo(int socket, int id, int player_count, bool direction){
  char packet[GAME_INFO_SIZE];
  uint8_t action = GAME_INFO_FLAG;
  memcpy(packet, &action, ACTION_B_COUNT);
  memcpy(packet+ACTION_B_COUNT, &id, sizeof(int));
  int next_player = calcNextPlayer(player_count, id, direction);
  memcpy(packet+ACTION_B_COUNT+sizeof(int), &next_player, sizeof(int));
  memcpy(packet+ACTION_B_COUNT+(sizeof(int)*2), &player_count, sizeof(int));

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
  return (bytes_sent == GAME_INFO_SIZE);
}

bool sendHandInfo(int socket, std::vector<Card> cards){
  //given a deck of 108 cards with 16 bytes per card, max size is only ever 1728 + 1 for hand indicator + 1 for for hand size int
  char packet[1730];
  uint8_t action = HAND_INFO_FLAG;
  memcpy(packet, &action, ACTION_B_COUNT);

  //safe as there is only 108 cards, something has gone horribly wrong otherwise lol
  uint8_t hand_size = (uint8_t) cards.size();
  memcpy(packet+ACTION_B_COUNT, &hand_size, sizeof(uint8_t));

  char cur_color;
  int8_t cur_num;

  int base_offset = ACTION_B_COUNT + sizeof(uint8_t);
  int offset = sizeof(char) + sizeof(int8_t);

  for(uint8_t i = 0; i < hand_size; i++){
    cur_color = cards.at(i).color;
    cur_num = cards.at(i).value;
    memcpy(packet+base_offset+(i*offset), &cur_color, sizeof(char));
    memcpy(packet+base_offset+(i*offset)+sizeof(char), &cur_num, sizeof(int8_t));
  }
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
  return (bytes_sent == packet_size);
}

bool sendOppInfo(int socket, std::vector<Player> players){
  int bytes_sent = 0;
  int packet_length = 73; //1 for the flag, 2 ints per player, less current player)
  char packet[73];
  uint8_t action = OPPONENT_INFO;
  memcpy(packet, &action, ACTION_B_COUNT);
  for(int i = 0; i < (int)players.size(); i++){
    if(i != socket){
      int count = (int)players.at(i).hand.size();
      memcpy(packet+ACTION_B_COUNT+(i*sizeof(int)*2), &i, sizeof(int));
      memcpy(packet+ACTION_B_COUNT+sizeof(int)+(i*sizeof(int)*2), &count, sizeof(int));
    }
  }
  int current_send = 0;
  while(bytes_sent != packet_length){
    current_send = send(socket,packet+bytes_sent,packet_length-bytes_sent,0);
    if (current_send == -1){
      return current_send;
    }
    bytes_sent += current_send;
  }
  return (bytes_sent == packet_length); //(bytes_sent == packet_length); 
}

bool sendTopCard(int socket, Card card){
  char packet[TOP_INFO_SIZE];
  uint8_t action = TOP_INFO_FLAG;
  int8_t val = card.value;
  char color = card.color;
  memcpy(packet, &action, ACTION_B_COUNT);
  memcpy(packet+ACTION_B_COUNT, &val, sizeof(int8_t));
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
  return (bytes_sent == TOP_INFO_SIZE);
}

int main(int argc, char* argv[]){
  const char* SERVER_PORT= argv[1];
  int player_count = 0;
  int max_player_count = atoi(argv[2]);

  char recvMsg[3] = {'\0'}; //only need at most 3 bytes lol, 1 for the flag, 2 for the card, and 3 for the color of the wild

  Deck uno;

  fd_set allSockets, callSet;
  FD_ZERO(&allSockets);
  FD_ZERO(&callSet);
  uint8_t action = 0;
  int listenSocket = bind_and_listen(SERVER_PORT);
  FD_SET(listenSocket, &allSockets);
  int maxSocket = listenSocket;
  while(player_count < max_player_count){
    callSet = allSockets;
    int numS = select(maxSocket + 1, &callSet, nullptr, nullptr, nullptr);
    if (numS < 0) {
      perror("ERROR in select() call");
      return -1;
    }
    for (int s = 3; s <= maxSocket; s++) {
      if (!FD_ISSET(s, &callSet)) {
        continue;
      }
      if (s == listenSocket) {
        int as = accept(s, nullptr, nullptr);
        if (as == -1) {
          std::perror("ERROR in accept() call");
          return -1;
        }
        FD_SET(as, &allSockets);
        maxSocket = find_max_fd(&allSockets);
        std::cout << "Socket " << listenSocket << " connected\n";
        struct Player player;
        player.socketDesc = as;
        uno.addPlayer(player);
        player_count++;
        continue;
      }

    }
  }

  std::cout << "game setup\n";

  uno.generateDeck();
  uno.shuffle();
  uno.dealCards(player_count);
  bool clock = true;
  int i = 0;
  while(true){
loopstart:
    action = 0;
    int8_t card_position = -1;
    char wild_color = 'E';
    i = calcNextPlayer(player_count, i, clock);
    Card top = uno.discard_pile.back();
    uno.printCard(top);
    std::cout << '\n';
    if(sendGameInfo(uno.players.at(i).socketDesc, i, player_count, clock)){
      std::cout << "game info sent\n";
    }
    else{
      std::cerr << "game info send error\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if(sendHandInfo(uno.players.at(i).socketDesc, uno.players.at(i).hand)){
      std::cout << "hand info sent\n";
    }
    else{
      std::cerr << "hand info send error\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if(sendOppInfo(uno.players.at(i).socketDesc, uno.players)){
      std::cout << "Opponent info sent\n";
    }
    else{
      std::cerr << "opponent info send error\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if(sendTopCard(uno.players.at(i).socketDesc, top)){
      std::cout << "top card sent\n";
    }
    else{
      std::cerr << "top card send error\n";
    }

    int s = uno.players.at(i).socketDesc;
    int rec = recv(s, recvMsg, sizeof(recvMsg), 0);
    uno.printHand(i);
    if (rec < 0) {
      std::perror("ERROR in recv() call");
      close(s);
      exit(1);
    } else if (rec == 0) {
      std::cout << "Socket " << s << " closed\n";
      FD_CLR(s, &allSockets);
      continue;
    }
    else{
      std::cout << "Socket val " << rec << "for " << s << "alive\n";
      memcpy(&action, recvMsg, sizeof(ACTION_B_COUNT));
      switch(action){
        case(DRAW_COMMAND):
          uno.drawCard(i);
          goto loopstart;
        case(PLAY_COMMAND):
          memcpy(&card_position, recvMsg+ACTION_B_COUNT, sizeof(int8_t));
          break;
        case(PLAY_WILD_COMMAND):
          memcpy(&wild_color, recvMsg+ACTION_B_COUNT+sizeof(int8_t), sizeof(char));
          break;
        default:
          std::cerr << "bad client respsonse\n";
          break;
      }
    }
    if(!uno.processInput(i, card_position)){
      uno.drawCard(i);
      continue;
    }
    if(uno.discard_pile.back().color == 'v'){
      std::cout << "Player: " << " wins!\n";
      //victory statement needs to be updated
      break;
    }
    //might cause a bug if the wild is named as the same card, but worry about later
    // checks if a card has actually been played, then process from there
    if(!((top.color == uno.discard_pile.back().color) && top.value == uno.discard_pile.back().value)){
      top = uno.discard_pile.back();
      int val = top.value;

      if(val == -3){
        clock = !clock;
        //swap direction
      }
      else if(val == -2){
        i = calcNextPlayer(player_count, i, clock);
        uno.drawCard(i);
        uno.drawCard(i);
        //skip next player and force them to draw
      }
      else if(val == -1){
        i = calcNextPlayer(player_count, i, clock);
        //skip next player
      }
      else if(val == -5){
        i = calcNextPlayer(player_count, i, clock);
        for(int d = 0; d < 4; d++)
          uno.drawCard(i);
        if(!checkWildInput(wild_color))
          wild_color = 'E';
        Card dummy;
        dummy.color = wild_color;
        dummy.value = 10;
        uno.discard_pile.push_back(dummy);

        //skip next turn, draw them 4, and make a dummy color
      }
      else if(val == -4){
        if(!checkWildInput(wild_color))
          wild_color = 'E';
        Card dummy;
        dummy.color = wild_color;
        dummy.value = 10;
        uno.discard_pile.push_back(dummy);
        //make a dummy card to indicate next color
      }
    }
  }
}

