#include "server_uno.h"
#include "cards.h"
#include <vector>
#include <algorithm>
#include <random>
#include <ranges>
#include <iostream>

void Deck::drawCard(int player){
  if(draw_pile.size() == 0){
    if(discard_pile.size() == 1)
      return;
    //don't draw or shuffle if we have no cards left, some poor sucker has everything in hand, sheesh
    Card top = discard_pile.back();
    discard_pile.pop_back();
    //save and remove last card from discard pile, then merge discard and draw, and then shuffle
    draw_pile.insert(draw_pile.end(), discard_pile.begin(), discard_pile.end());
    discard_pile.clear();
    discard_pile.push_back(top);
    shuffle();
  }
	players.at(player).hand.push_back(draw_pile.back());
	draw_pile.pop_back();
}

void Deck::printHand(int player){
	//must be a player num <= player count
	for(size_t i = 0; i < players.at(player).hand.size(); i++){
		printCard(players.at(player).hand.at(i));
	}
	std::cout << '\n';
}

void Deck::dealCards(std::vector<std::string> names){
	if(names.size() == 0 || names.size() > 10)
		std::cerr << "Wrong number of players\n";
	for(size_t i = 0; i < names.size(); i++){
		Player person;
		person.name = names.at(i);
		//this is always safe since max player count 10 is smaller than deck size, so no need to check
		for(int c = 0; c < 7; c++){
			person.hand.push_back(draw_pile.back());
			draw_pile.pop_back();
		}
		players.push_back(person);
	}
	discard_pile.push_back(draw_pile.back());
	draw_pile.pop_back();
}

void Deck::shuffle(){
	int seed = 42;
	auto rng = std::default_random_engine(seed);
	std::ranges::shuffle(draw_pile, rng);
}

bool Deck::processInput(int player, int input){
	if(input < 0)
		return false;
	if((int)players.at(player).hand.size() <= input)
		return false;
	Card play = players.at(player).hand.at(input);
	if(play.value == discard_pile.back().value || play.color == discard_pile.back().color || play.color == 'w'){
		if(discard_pile.back().value == 10)
      discard_pile.pop_back();
    //remove the wild card leftovers
    discard_pile.push_back(play);
		players.at(player).hand.erase (players.at(player).hand.begin()+input);
    if(players.at(player).hand.size() == 0){
      Card win;
      win.value = 99;
      win.color = 'v';
      discard_pile.push_back(win);
    }
		return true;
	}
	return false;
}

void Deck::generateDeck(){
	char color;
	//generate and add the non wild cards, -1 for skip, -2 for draw 2, -3 for reverse
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
		for(int i = -3; i < 10; i++){
			Card current;
			current.color = color;
			current.value = i;
			if(i != 0)
				draw_pile.push_back(current);
			draw_pile.push_back(current);
		}
	}
	color = 'w';
	int number = -4;
	for(int i = 0; i < 2; i++){
		for(int j = 0; j < 4; j++){
			Card current;
			current.color = color;
			current.value = number;
			draw_pile.push_back(current);
		}
		number = -5;
	}
}
