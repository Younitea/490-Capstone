#ifndef CARD_H
#define CARD_H

#include <vector>
#include <string>

struct Card{
	char color;
	int value;
};

struct Player{
	std::vector<Card> hand;
	std::string name;
};

#endif

//value 0 to 9, -1 for skip, -2 for draw, -3 for reverse r, g, b, y
//then value is -4 for wild, and -5 for draw 4
//
//when processing, a fake card will be added for the wild with inputted color and value 10, this will be removed after the next valid play
