#include "server_uno.h"
#include "cards.h"
#include <vector>
#include <algorithm>
#include <random>
#include <ranges>
#include <iostream>

void Deck::dealCards(std::vector<std::string> names){
  if(names.size() == 0 || names.size() > 10)
    std::cerr << "Wrong number of players\n";
  for(size_t i = 0; i < names.size(); i++){
    Player person;
    person.name = names.at(i);
    for(int c = 0; c < 7; c++){
      //TODO person.hand.push_back(//NEED to remove top card from stack!
    }
    players.push_back(person);
  }

}

void Deck::shuffle(){
  int seed = 42;
  auto rng = std::default_random_engine(seed);
  std::ranges::shuffle(draw_pile, rng);
  for(size_t i = 0; i < draw_pile.size(); i++) //test output, remove later
    std::cout << draw_pile.at(i).value << draw_pile.at(i).color << '\n';
}

void Deck::generateDeck(){
  char color;
  //generate and add the non wild cards, -1 for skip, -2 for draw 2
  for(int c = 0; c < 4; c++){
    switch(c){
      case(0):
        color = 'r';
        break;
      case(1):
        color = 'b';
        break;
      case(2):
        color = 'y';
        break;
      case(3):
        color = 'g';
    }
    for(int i = -2; i < 10; i++){
      Card current;
      current.color = color;
      current.value = i;
      if(i != 0)
        draw_pile.push_back(current);
      draw_pile.push_back(current);
    }
  }
  color = 'w';
  int number = 0;
  for(int i = 0; i < 2; i++){
    for(int j = 0; j < 4; j++){
      Card current;
      current.color = color;
      current.value = number;
      draw_pile.push_back(current);
    }
    number = -4;
  }
}
