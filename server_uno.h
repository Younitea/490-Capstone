#include "cards.h"
class Deck{
  public:
    vector<Cards> discard_pile;
    void shuffle;
    void generateDeck(draw_pile);
  private:
    vector<Cards> draw_pile;
};
