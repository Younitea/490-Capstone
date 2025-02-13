#ifndef SERVER_UNO_H
#define SERVER_UNO_H

#include <vector>
#include <string>
#include "cards.h"
class Deck{
  public:
    std::vector<Card> discard_pile;
    void shuffle();
    void generateDeck();
    void dealCards(std::vector<std::string> names);
  private:
    std::vector<Card> draw_pile;
    std::vector<Player> players;
};

#endif
