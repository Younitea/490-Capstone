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
		void dealCards(int player_count); //start of the game only
		void printHand(int player_num);
		void printCard(Card card){
			std::cout << (int)card.value << card.color << '\n';
		}
		void drawCard(int player);
		bool processInput(int player, int input);
    void addPlayer(struct Player player);
    std::vector<Player> players;
	private:
		std::vector<Card> draw_pile;
};

#endif
