#include "cards.h"
#include "server_uno.h"

#include <iostream>
#include <vector>

int main(){
  Deck uno;
  uno.generateDeck();
  uno.shuffle();
}
