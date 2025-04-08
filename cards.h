#ifndef CARD_H
#define CARD_H

#include <vector>
#include <string>
#include "netinet/in.h"

#define DRAW_COMMAND 1
#define DRAW_PACKET_SIZE 1

#define PLAY_COMMAND 2
#define PLAY_PACKET_SIZE 2

#define PLAY_WILD_COMMAND 3
#define PLAY_WILD_SIZE 3

#define GAME_INFO_SIZE 13
#define GAME_INFO_FLAG 4

#define HAND_INFO_FLAG 5

#define OPPONENT_INFO 6

#define TOP_INFO_SIZE 3
#define TOP_INFO_FLAG 7

#define ACTION_B_COUNT 1

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
