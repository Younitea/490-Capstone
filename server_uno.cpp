#include "server_uno.h"
#include "cards.h"
void generateDeck(&deck)
  char color;
  //generate and add the non wild cards, -1 for skip, -2 for draw 2
  for(int c = 0; c < 4; i++){
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
      Card current = (color, i);
      if(i != 0)
        deck.push_back(current);
      deck.push_back(current);
    }
  }
  color = 'w';
  int number = 0;
  for(int i = 0; i < 2; i++){
    for(int j = 0; j < 4; j++)
      Card current = (color, number);
    number = -4
  }
}
