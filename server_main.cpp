#include "cards.h"
#include "server_uno.h"

#include <iostream>
#include <vector>

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

int main(){
  Deck uno;
  uno.generateDeck();
  uno.shuffle();
  std::vector<std::string> players = {"1","2","3"};
  uno.dealCards(players);
  int input = 0;
  bool clock = true;
  int player_count = (int)players.size();
  int i = 0;
  while(input != -1){
    i = calcNextPlayer(player_count, i, clock);
    Card top = uno.discard_pile.back();
    uno.printCard(top);
    std::cout << '\n';
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
      std::cout << "Player: " << players.at(i) << " wins!\n";
      break;
    }
    //might cause a bug if the wild is named as the same card, but worry about latr
    // checks if a card has actually been played, then process from there
    if(!((top.color == uno.discard_pile.back().color) && top.value == uno.discard_pile.back().value)){
      top = uno.discard_pile.back();
      int val = top.value;
      
      std::cout << "made it\n";

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
