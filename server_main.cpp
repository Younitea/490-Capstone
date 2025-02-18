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
	int player_count = (int)players.size();
	int i = 0;
	while(input != -1){
		if(uno.clock){
			i++;
			if(i >= player_count)
				i = 0;
		}
		else{
			i--;
			if(i < 0)
				i = (player_count - 1);
		}
		uno.printCard(uno.discard_pile.back());
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
	}

}
