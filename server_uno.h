#ifndef SERVER_UNO_H
#define SERVER_UNO_H

#include <vector>
#include <string>
#include <iostream>
#include "cards.h"
class Deck{
	public:
		std::vector<Card> discard_pile;
		void shuffle();
		void generateDeck();
		void dealCards(std::vector<std::string> names); //start of the game only
		void printHand(int player_num);
		void printCard(Card card){
			std::cout << card.value << card.color << '\n';
		}
	private:
		std::vector<Card> draw_pile;
		std::vector<Player> players;
};

#endif
