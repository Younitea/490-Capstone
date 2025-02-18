#include "cards.h"
#include "server_uno.h"

#include <iostream>
#include <vector>

int main(){
  Deck uno;
  uno.generateDeck();
  uno.shuffle();
  std::vector<std::string> players = {"1","2"};
  uno.dealCards(players);
  int input = 0;
  while(input != -1){
    for(size_t i = 0; i < players.size(); i++){
      uno.printCard(uno.discard_pile.back());
      std::cout << '\n';
      uno.printHand(i);
      std::cin >> input;
      if(input == -1)
        break;
      //input validation needed
      //uno.process_input(i, input);
    }
  }
}
