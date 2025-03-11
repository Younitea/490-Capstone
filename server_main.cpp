#include "cards.h"
#include "server_uno.h"
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <unistd.h>

#define MAX_PENDING 5
#define HAND_INFO_FLAG 4
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

char getWildInput(){
  // needs to be a r,g,b,y will break otherwise, will do more validation checking later
  std::cout << "input color\n";
  char color;
  std::cin >> color;
  return color;
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
  return (bytes_sent == 1730);
}


int main(int argc, char* argv[]){
  const char* SERVER_PORT= argv[1];
  int player_count = 0;

  char recvMsg[1205] = {'\0'};

  Deck uno;

  fd_set allSockets, callSet;
  FD_ZERO(&allSockets);
  FD_ZERO(&callSet);
  uint8_t action = 0;
  int listenSocket = bind_and_listen(SERVER_PORT);
  FD_SET(listenSocket, &allSockets);
  int maxSocket = listenSocket;
  bool game_setup = false;
  while(!game_setup){
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
      else{
        int rec = recv(s, recvMsg, sizeof(recvMsg), 0);
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
          memcpy(&action, recvMsg, sizeof(action));
          for (int i = 0; i < player_count; i++) {
            if (uno.players.at(i).socketDesc == s){
              if(action == 10){
                game_setup = true;
                recvMsg[0] = '\0';
              }
            }
          }
        }
      }
    }
  }

  std::cout << "game setup\n";

  uno.generateDeck();
  uno.shuffle();
  uno.dealCards(player_count);
  int input = 0;
  bool clock = true;
  int i = 0;
  while(input != -1){
    i = calcNextPlayer(player_count, i, clock);
    Card top = uno.discard_pile.back();
    uno.printCard(top);
    std::cout << '\n';
    if(sendHandInfo(uno.players.at(i).socketDesc, uno.players.at(i).hand)){
      std::cout << "hand info sent\n";
    }
    else{
      std::cerr << "send error\n";
    }

    uno.printHand(i);

    std::cin >> input;
    if(input == -1)
      break;
    //input validation needed
    int attempts = 0;
    while(!uno.processInput(i, input)){
      attempts++;
      if(attempts >= 3){
        uno.drawCard(i);
        break;
      }
      std::cin >> input;
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
        char wild = getWildInput();
        Card dummy;
        dummy.color = wild;
        dummy.value = 10;
        uno.discard_pile.push_back(dummy);
        //skip next turn, draw them 4, and make a dummy color
      }
      else if(val == -4){
        char wild = getWildInput();
        Card dummy;
        dummy.color = wild;
        dummy.value = 10;
        uno.discard_pile.push_back(dummy);
        //make a dummy card to indicate next color
      }
    }
  }
}

