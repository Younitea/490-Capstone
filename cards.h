#ifndef CARD_H
#define CARD_H

struct Card{
  char color;
  int value;
};

#endif

//value 0 to 9, -1 for skip, -2 for draw r, g, b, y
//wild's color is w before playing and then set to one above after 
//then value is 0 for wild, and -4 for draw 4, for number checking, set to -3 to represent any number
