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
}
