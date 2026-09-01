#ifndef FIRRST_HAND_OPEN_HPP
#define FIRRST_HAND_OPEN_HPP
#include "loveletter.hpp"
node::first_hand_open(int trash[8], int hand1, int hand2, bool barrier2, int open[3]) {
  for(int i = 0; i < 8; i++) {
    deck[i] = max_num[i] - trash[i];
  }
  this.hand1[0] = hand1;
  this.hand1[1] = 0;
  this.hand2 = hand2;
  this.barrier1 = false;
  this.barrier2 = barrier2;
  this.open[0] = open[0];
  this.open[1] = open[1];
  this.open[2] = open[2];
}
#endif