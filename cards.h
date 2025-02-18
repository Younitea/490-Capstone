#ifndef CARD_H
#define CARD_H

#include <vector>
#include <string>

struct Card{
	char color;
	int value;
	bool shuffle = true;
};

struct Player{
	std::vector<Card> hand;
	std::string name;
};

#endif

//value 0 to 9, -1 for skip, -2 for draw, -3 for reverse r, g, b, y
//wild's color is w before playing and then set to one above after DOESN'T WORK, need to be able to shuffle card back in
//then value is -4 for wild, and -5 for draw 4, 
