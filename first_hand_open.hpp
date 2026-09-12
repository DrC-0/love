#ifndef FIRST_HAND_OPEN_HPP
#define FIRST_HAND_OPEN_HPP
#include "belief_state_history.hpp"

bool is_openhand_loveletter(const belief_state& bs);
bool is_openhand_loveletter_hisp(int open[3], std::string history, bool rnd);
bool is_openhand_loveletter_his(int open[3], std::string his0, std::string his1, bool rnd);

// 開手ラブレター: 双方の手札が互いに既知で、まだ引く前 (手札1枚) の局面。
// open_e() / open_s() は手札が一意に定まらないとき 0 を返すので、判定は > 0。
// 兵士の判定に出てくる open_e() > 1 は「確定していて、かつカード1でない」という
// 別の条件 (兵士はカード1を宣言できない)。ここでその形を使ってはいけない。
bool is_openhand_loveletter(const belief_state& bs) {
  return bs.open_e() > 0 && bs.open_s() > 0 && bs.hand_s[1] == 0;
}

bool is_openhand_loveletter_hisp(int open[3], std::string history, bool rnd) {
  belief_state bs(open, history, rnd);
  return is_openhand_loveletter(bs);
}

bool is_openhand_loveletter_his(int open[3], std::string his0, std::string his1, bool rnd) {
  belief_state bs0(open, his0, rnd);
  belief_state bs1(open, his1, rnd);
  // bs0 は先手視点、bs1 は後手視点。それぞれの open_e() が相手の手札なので、
  // 両方 > 0 なら双方の手札が互いに既知。
  return bs0.open_e() > 0 && bs1.open_e() > 0 && bs0.hand_s[1] == 0 && bs1.hand_s[1] == 0;
}
#endif
