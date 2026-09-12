#ifndef FIRST_HAND_OPEN_HPP
#define FIRST_HAND_OPEN_HPP
#include "belief_state_history.hpp"

bool is_openhand_loveletter(const belief_state& bs);
bool is_openhand_loveletter_hisp(int open[3], std::string history, bool rnd);
bool is_openhand_loveletter_his(int open[3], std::string his0, std::string his1, bool rnd);

bool is_openhand_loveletter(const belief_state& bs) {
  return bs.open_e() > 1 && bs.open_s() > 1 && bs.hand_s[1] == 0;
}

bool is_openhand_loveletter_hisp(int open[3], std::string history, bool rnd) {
  belief_state bs(open, history, rnd);
  return is_openhand_loveletter(bs);
}

bool is_openhand_loveletter_his(int open[3], std::string his0, std::string his1, bool rnd) {
  belief_state bs0(open, his0, rnd);
  belief_state bs1(open, his1, rnd);
  return bs0.open_e() > 1 && bs1.open_e() > 1 && bs0.hand_s[1] == 0 && bs1.hand_s[1] == 0;
}
#endif
