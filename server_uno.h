#ifndef SERVER_UNO_H
#define SERVER_UNO_H

#include <vector>
#include "cards.h"
class Deck{
  public:
    std::vector<Card> discard_pile;
    void shuffle();
    void generateDeck();
  private:
    std::vector<Card> draw_pile;
};

#endif
