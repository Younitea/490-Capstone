#ifndef CARD_H
#define CARD_H

#include <vector>
#include <string>
#include "netinet/in.h"

struct Card{
	char color;
	int8_t value;
};

struct Player{
	std::vector<Card> hand;
	std::string name;
  uint32_t id = 0;
  int socketDesc = 0;
  sockaddr_in address{};
};

#endif

//value 0 to 9, -1 for skip, -2 for draw, -3 for reverse r, g, b, y
//then value is -4 for wild, and -5 for draw 4
//
//when processing, a fake card will be added for the wild with inputted color and value 10, this will be removed after the next valid play
