#ifndef MAX_NUM
const int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};
#define MAX_NUM
#endif
#ifndef BF_POSITION_HPP
#define BF_POSITION_HPP
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include <cstdlib>

#include "rnd_action.hpp"
#include "org_action.hpp"
// #include "endgame.hpp"

using namespace std;

extern Rnd_Perfect_Hash rph;
extern Org_Perfect_Hash oph;

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);
extern char action_to_char(int action, int card);
extern char wizard_to_char(int to, int trash, int draw);
extern char twonum_to_char(int card1, int card2);

bool commentablebfp = false;

struct bf_position {
  bool is_my_turn;
  bool is_sol_choice;
  bool is_wiz_choice;
  bool not7_flag_s; // 1bit
  bool not7_flag_e; // 1bit
  bool barrier_s; // 1bit
  bool barrier_e; // 1bit
  bool lt5_flag_s; // 1bit
  bool lt5_flag_e; // 1bit
  int open_flag_s; // 4bit
  int open_flag_e; // 4bit
  int sol_flag_s[2]; // 7bit
  int sol_flag_e[2]; // 7bit
  int hand_s[2]; // 6bit
  int trash[8]; // 14bit
  bf_position()
    : is_my_turn(false), is_sol_choice(false), is_wiz_choice(false), not7_flag_s(false), not7_flag_e(false),
      barrier_s(false), barrier_e(false), lt5_flag_s(false), lt5_flag_e(false), open_flag_s(0), open_flag_e(0),
      sol_flag_s{0, 0}, sol_flag_e{0, 0}, hand_s{0, 0}, trash{0, 0, 0, 0, 0, 0, 0, 0} {}
  bf_position(bool is_my_turn, bool is_sol_choice, bool is_wiz_choice, bool not7_flag_s, bool not7_flag_e, bool barrier_s, bool barrier_e,
              bool lt5_flag_s, bool lt5_flag_e, int open_flag_s, int open_flag_e,
              int sol_flag_s[2], int sol_flag_e[2], const int hand_s[2], const int trash[8])
    : is_my_turn(is_my_turn),
      is_sol_choice(is_sol_choice),
      is_wiz_choice(is_wiz_choice),
      not7_flag_s(not7_flag_s),
      not7_flag_e(not7_flag_e),
      barrier_s(barrier_s),
      barrier_e(barrier_e),
      lt5_flag_s(lt5_flag_s),
      lt5_flag_e(lt5_flag_e),
      open_flag_s(open_flag_s),
      open_flag_e(open_flag_e) {
    std::copy(sol_flag_s, sol_flag_s + 2, this->sol_flag_s);
    std::copy(sol_flag_e, sol_flag_e + 2, this->sol_flag_e);
    std::copy(hand_s, hand_s + 2, this->hand_s);
    std::copy(trash, trash + 8, this->trash);
  }
  bf_position(int open[3], std::string history, bool rnd);
  int deck_or_hand_e(int i) const;
  bool hand_e(int i) const;
  bool hand_s_est(int i) const;
  int open_e() const;
  int open_s() const;
  bool deck(int i) const;
  bool have_s(int card) const;
  int other_hand_s(int card) const;
  int count_deck() const;
  int hand_e_max() const;
  int hand_e_min() const;
  int deck_or_hand_e_min() const;
  void add_sol_s(int card);
  void add_sol_e(int card);
  void reset_flag(bool is_self);
  void print() const;
};

int trash_and_hand_s(const int i, const int hand[2], const int trash[8]);
int deck_or_hand_e(const int i, const int hand[2], const int trash[8]);
bool hand_e(const int i, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e);
int open_e(const int hand[2], const int trash[8], const int open_flag_s, const int sol_flag_s[2], const bool lt5_flag_s, const bool not7_flag_s);
bool deck(const int i, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e);
bool in(const int hand[2], const int target);
int other(const int hand[2], const int target);
int count_deck(const int hand[2], const int trash[8]);

bf_position reset_flag_by_use(const bf_position& bfp, bool is_self, int card);
std::pair<int, int> is_win(const bf_position& bfp);
std::pair<int, int> is_terminated_win(const bf_position& bfp);
std::pair<bool, int> use_win(const bf_position& bfp, int card);
std::pair<bool, int> draw_win(const bf_position& bfp);
std::pair<bool, int> enemy_turn_win(const bf_position& bfp);
std::pair<bool, int> sol_win(const bf_position& bfp, int card);
std::pair<bool, int> wiz_win(const bf_position& bfp, bool to0p);
std::vector<bf_position> ef_wizard(const bf_position& bfp, bool to_0p);
bf_position draw(const bf_position& bfp, int draw_card);
bf_position swap_player(const bf_position& bfp, const int hand);
unsigned char action2char(int x, bool rnd);
std::string actions_to_string(const std::vector<int>& actions, bool rnd);
std::string get_actions_history(std::string s, bool rnd);
void output_actions_history(std::string s, bool rnd);
std::vector<std::pair<int, int>> is_lose(const bf_position& bfp);
std::pair<bool, int> use_lose(const bf_position& bfp, int card);
std::pair<bool, int> wiz_lose(const bf_position& bfp, bool to0p);
std::vector<int> able_actions(const bf_position& bfp, int card, bool is_second_player);
[[noreturn]] void exit_with_print(const bf_position& bfp, const char* context);
void validate_hand_e_candidate(const bf_position& bfp, const char* context);
int action_count(const bf_position& bfp);
bool is_openhand_loveletter(const bf_position& bfp);
bool is_openhand_loveletter_hisp(int open[3], std::string history, bool rnd);
bool is_openhand_loveletter_his(int open[3], std::string his0, std::string his1, bool rnd);

int trash_and_hand_s(const int i, const int hand[2], const int trash[8]) {
  return trash[i] + (hand[0] == i + 1 ? 1 : 0) + (hand[1] == i + 1 ? 1 : 0);
}

int deck_or_hand_e(const int i, const int hand[2], const int trash[8]) {
  return max_num[i] - trash_and_hand_s(i, hand, trash);
}

bool hand_e(const int i, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e) {
  if(open_flag_e > 0) { //手札が確定している場合
    if(open_flag_e == i + 1) {
      return true;
    } else {
      return false;
    }
  } else if(sol_flag_e[0] > 1 && sol_flag_e[0] == i + 1) {
    return false;
  } else if(sol_flag_e[1] > 1 && sol_flag_e[1] == i + 1) {
    return false;
  } else if(lt5_flag_e && i + 1 >= 5) { //前のターンに7を出したときの,5以上
    return false;
  } else if(not7_flag_e && i + 1 == 7) { //前のターンに5を出したときの,7
    return false;
  } else {
    return deck_or_hand_e(i, hand, trash) > 0;
  }
}

int open_e(const int hand[2], const int trash[8], const int open_flag_s, const int sol_flag_s[2], const bool lt5_flag_s, const bool not7_flag_s) {
  int card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_e(i, hand, trash, open_flag_s, sol_flag_s, lt5_flag_s, not7_flag_s)) {
      if(card == 0) {
        card = i + 1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

bool deck(const int i, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e) {
  int open_card = open_e(hand, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag_e);
  return deck_or_hand_e(i, hand, trash) > (i + 1 == open_card ? 1 : 0);
}

bool in(const int hand[2], const int target) {
  return hand[0] == target || hand[1] == target;
}

int other(const int hand[2], const int target) {
  if(hand[0] == target) return hand[1];
  else if(hand[1] == target) return hand[0];
  return 0;
}

int count_deck(const int hand[2], const int trash[8]) {
  int count = 0;
  for(int i = 0; i < 8; i++) {
    count += deck_or_hand_e(i, hand, trash);
  }
  return count - 1;
}

bf_position::bf_position(int open[3], string history, bool rnd = true)
  : is_my_turn(false), is_sol_choice(false), is_wiz_choice(false),
    not7_flag_s(false), not7_flag_e(false), barrier_s(false), barrier_e(false), lt5_flag_s(false), lt5_flag_e(false), open_flag_s(0),
    open_flag_e(0), sol_flag_s{0, 0}, sol_flag_e{0, 0}, hand_s{0, 0}, trash{0, 0, 0, 0, 0, 0, 0, 0} {
  for(int i = 0; i < 3; i++) {
    trash[open[i] - 1] += 1;
  }
  long unsigned int head = 0;
  bool is_second_player = false;
  while(head < history.size()) {
    string action;
    if(rnd) action = rph.get_action((unsigned char)history[head]);
    else action = oph.get_action((unsigned char)history[head]);
    int c2a = char_to_action(action[0]);
    head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;

    if(num1 == 1) {
      hand_s[0] = num2 + 1;
      is_my_turn = true;
      is_second_player = false;
    } else if(num1 == 2) {
      hand_s[0] = num2 + 1;
      is_my_turn = false;
      is_second_player = true;
    } else if(num1 == 3) {
      hand_s[1] = num2 + 1;
      barrier_s = false;
    } else if(num1 == 4) {
      is_sol_choice = false;
      is_wiz_choice = false;
      if(is_my_turn) { // player1のカード使用
        trash[num2] += 1;

        if(hand_s[0] == num2 + 1) { //手札を減らす
          hand_s[0] = hand_s[1];
          hand_s[1] = 0;
        } else if(hand_s[1] == num2 + 1) {
          hand_s[1] = 0;
        }

        //手札の候補のリセット
        barrier_s = false;
        *this = reset_flag_by_use(*this, true, num2 + 1);

        if(num2 + 1 == 1) {
          //相手が保護されている場合、宣言自体が行われないため選択ノードにはならない
          if(!barrier_e) {
            if(action.size() > 1) {
              int c2t = char_to_twonum(action[1]);
              int choice = (c2t % 10) + 1;
              if(choice > 1) add_sol_e(choice);
            } else is_sol_choice = true;
          }
        } else if(num2 + 1 == 2 && !barrier_e) {
          int c2t = char_to_twonum(action[1]);
          open_flag_e = (c2t % 10) + 1;
        } else if(num2 + 1 == 3 && !barrier_e) {
          int c2t = char_to_twonum(action[1]);
          open_flag_e = (c2t % 10) + 1;
          open_flag_s = (c2t % 10) + 1;
        } else if(num2 + 1 == 4) {
          barrier_s = true;
        } else if(num2 + 1 == 5) {
          if(action.size() > 1) {
            not7_flag_s = true;
            int c2w = char_to_wizard(action[1]);
            int to = c2w / 100;
            int trashcard = (c2w / 10) % 10;
            int draw = c2w % 10;
            if(is_second_player == to) {
              trash[trashcard] += 1;
              hand_s[0] = draw + 1;
              reset_flag(true); //自分のフラグリセット
            } else if(!barrier_e) {
              trash[trashcard] += 1;
              reset_flag(false); //相手のフラグリセット
            }
          } else is_wiz_choice = true;
        } else if(num2 + 1 == 6 && !barrier_e) {
          int c2t = char_to_twonum(action[1]);
          hand_s[0] = (c2t % 10) + 1;
          reset_flag(true);
          reset_flag(false);
          open_flag_s = (c2t % 10) + 1;
          open_flag_e = (c2t / 10) + 1;
        } else if(num2 + 1 == 7) {
          lt5_flag_s = true;
        } else {
        }
      } else { // player2のカード使用
        trash[num2] += 1;

        //手札の候補のリセット
        barrier_e = false;
        *this = reset_flag_by_use(*this, false, num2 + 1);
        if(num2 + 1 == 1 && !barrier_s && action.size() > 1) {
          int c2t = char_to_twonum(action[1]);
          int choice = (c2t % 10) + 1;
          if(choice > 1) add_sol_s(choice);
        } else if(num2 + 1 == 2 && !barrier_s) {
          int c2t = char_to_twonum(action[1]);
          open_flag_s = (c2t % 10) + 1;
        } else if(num2 + 1 == 3 && !barrier_s) {
          int c2t = char_to_twonum(action[1]);
          open_flag_e = (c2t % 10) + 1;
          open_flag_s = (c2t % 10) + 1;
        } else if(num2 + 1 == 4) {
          barrier_e = true;
        } else if(num2 + 1 == 5) {
          not7_flag_e = true;
          int c2w = char_to_wizard(action[1]);
          int to = c2w / 100;
          int trashcard = (c2w / 10) % 10;
          int draw = c2w % 10;
          if(is_second_player != to) {
            trash[trashcard] += 1;
            reset_flag(false); //相手のフラグリセット
          } else if(!barrier_s) {
            trash[trashcard] += 1;
            hand_s[0] = draw + 1;
            reset_flag(true); //自分のフラグリセット
          }
        } else if(num2 + 1 == 6 && !barrier_s) {
          int c2t = char_to_twonum(action[1]);
          hand_s[0] = (c2t % 10) + 1;
          reset_flag(true);
          reset_flag(false);
          open_flag_s = (c2t % 10) + 1;
          open_flag_e = (c2t / 10) + 1;
        } else if(num2 + 1 == 7) {
          lt5_flag_e = true;
        }
      }
      is_my_turn = !is_my_turn;
    }
  }
}

int bf_position::deck_or_hand_e(int i) const {
  return ::deck_or_hand_e(i, hand_s, trash);
}

bool bf_position::hand_e(int i) const {
  return ::hand_e(i, hand_s, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag_e);
}

bool bf_position::hand_s_est(int i) const {
  int enemy_hand[2] = {open_e(), 0};
  return ::hand_e(i, enemy_hand, trash, open_flag_s, sol_flag_s, lt5_flag_s, not7_flag_s);
}

int bf_position::open_e() const {
  int card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_e(i)) {
      if(card == 0) {
        card = i + 1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

int bf_position::open_s() const {
  int card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_s_est(i)) {
      if(card == 0) {
        card = i + 1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

bool bf_position::deck(int i) const {
  int open_card = open_e();
  return deck_or_hand_e(i) > (i + 1 == open_card ? 1 : 0);
}

bool bf_position::have_s(int card) const {
  return hand_s[0] == card || hand_s[1] == card;
}

int bf_position::other_hand_s(int card) const {
  if(hand_s[0] == card) {
    return hand_s[1];
  } else if(hand_s[1] == card) {
    return hand_s[0];
  } else {
    return 0;
  }
}

int bf_position::count_deck() const {
  int count = 0;
  for(int i = 0; i < 8; i++) {
    count += deck_or_hand_e(i);
  }
  return count - 1;
}

int bf_position::hand_e_max() const {
  int max_card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_e(i)) {
      max_card = i + 1;
    }
  }
  return max_card;
}

int bf_position::hand_e_min() const {
  int min_card = 0;
  for(int i = 0; i < 8; i++) {
    if(hand_e(i)) {
      if(min_card == 0 || i + 1 < min_card) {
        min_card = i + 1;
      }
    }
  }
  return min_card;
}

int bf_position::deck_or_hand_e_min() const {
  int min_card = 0;
  for(int i = 0; i < 8; i++) {
    if(deck_or_hand_e(i) > 0) {
      if(min_card == 0 || i + 1 < min_card) {
        min_card = i + 1;
      }
    }
  }
  return min_card;
}

void bf_position::add_sol_s(int card) {
  if(sol_flag_s[0] == 0) {
    sol_flag_s[0] = card;
  } else {
    if(sol_flag_s[0] < card) {
      sol_flag_s[1] = card;
    } else if(sol_flag_s[0] > card) {
      sol_flag_s[1] = sol_flag_s[0];
      sol_flag_s[0] = card;
    }
  }
}

void bf_position::add_sol_e(int card) {
  if(sol_flag_e[0] == 0) {
    sol_flag_e[0] = card;
  } else {
    if(sol_flag_e[0] < card) {
      sol_flag_e[1] = card;
    } else if(sol_flag_e[0] > card) {
      sol_flag_e[1] = sol_flag_e[0];
      sol_flag_e[0] = card;
    }
  }
}

void bf_position::reset_flag(bool is_self) {
  if(is_self) {
    not7_flag_s = false;
    lt5_flag_s = false;
    open_flag_s = 0;
    sol_flag_s[0] = 0;
    sol_flag_s[1] = 0;
  } else {
    not7_flag_e = false;
    lt5_flag_e = false;
    open_flag_e = 0;
    sol_flag_e[0] = 0;
    sol_flag_e[1] = 0;
  }
}

bf_position reset_flag_by_use(const bf_position& bfp, bool to_self, int card) {
  struct bf_position next_bfp = bfp;
  if(to_self) {
    if(bfp.open_flag_s > 0 && bfp.open_flag_s == card) {
      next_bfp.open_flag_s = 0;
    }
    if(bfp.sol_flag_s[1] != 0 && bfp.sol_flag_s[1] != card) {
      next_bfp.sol_flag_s[1] = 0;
    }
    if(bfp.sol_flag_s[0] != 0 && bfp.sol_flag_s[0] != card) {
      next_bfp.sol_flag_s[0] = next_bfp.sol_flag_s[1];
      next_bfp.sol_flag_s[1] = 0;
    }
    if(bfp.lt5_flag_s && card < 5) {
      next_bfp.lt5_flag_s = false;
    }
    next_bfp.not7_flag_s = false; //対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

  } else {
    if(bfp.open_flag_e > 0 && bfp.open_flag_e == card) {
      next_bfp.open_flag_e = 0;
    }
    if(bfp.sol_flag_e[1] != 0 && bfp.sol_flag_e[1] != card) {
      next_bfp.sol_flag_e[1] = 0;
    }
    if(bfp.sol_flag_e[0] != 0 && bfp.sol_flag_e[0] != card) {
      next_bfp.sol_flag_e[0] = next_bfp.sol_flag_e[1];
      next_bfp.sol_flag_e[1] = 0;
    }
    if(bfp.lt5_flag_e && card < 5) {
      next_bfp.lt5_flag_e = false;
    }
    next_bfp.not7_flag_e = false;
  }
  return next_bfp;
}

std::pair<int, int> is_terminated_win(const bf_position& bfp) {
  if(bfp.have_s(7) && bfp.hand_s[0] + bfp.hand_s[1] >= 12) {
    return {0, 0};
  }
  if(bfp.count_deck() < 2 && bfp.hand_s[1] == 0 && !bfp.is_wiz_choice && !bfp.is_sol_choice) {
    int max = bfp.hand_e_max();
    // if(max == 0) {cout << "Error: max card is 0" << endl; bfp.print(); exit(1);}
    if(max < bfp.hand_s[0]) return {bfp.hand_s[0], 0};
    else return {0, 0};
  }
  if(!bfp.barrier_e && bfp.hand_s[1] > 0) {
    if(bfp.have_s(1) && bfp.open_e() > 1) {
      return {1, 1};
    }
    if(bfp.have_s(3) && bfp.hand_e_max() < bfp.other_hand_s(3)) {
      return {3, 1};
    }
    if(bfp.have_s(5) && bfp.open_e() == 8) {
      return {5, 1};
    }
  }
  return {-1, 0};
}

std::pair<int, int> is_win(const bf_position& bfp) {
  auto t = is_terminated_win(bfp);
  if(t.first != -1) return t;

  std::pair<int, int> res;
  if(bfp.hand_s[1] == 0 && bfp.is_sol_choice) {
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bfp, "sol");
    for(int i = 1; i < 8; i++) {
      if(bfp.hand_e(i)) {
        auto res_sol = sol_win(bfp, i + 1);
        if(res_sol.first) {
          has_true = true;
          min_t = std::min(min_t, res_sol.second);
        }
      }
    }
    return {has_true, has_true ? min_t + 1 : 0};

  } else if(bfp.hand_s[1] == 0 && bfp.is_wiz_choice) {
    auto res_self = wiz_win(bfp, true);
    auto res_enemy = wiz_win(bfp, false);

    bool has_true = res_self.first || res_enemy.first;

    int min_t = 1e9;
    if(res_self.first) min_t = std::min(min_t, res_self.second);
    if(res_enemy.first) min_t = std::min(min_t, res_enemy.second);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(bfp.is_my_turn && bfp.hand_s[1] != 0) {
    // 手札が2枚あり、両方同じカードの場合（片方だけ評価して無駄を省く）
    if(bfp.hand_s[0] == bfp.hand_s[1]) {
      res = use_win(bfp, bfp.hand_s[0]);
    }
    // 手札が2枚あり、違うカードの場合（ORノードの評価）
    else {
      auto res0 = use_win(bfp, bfp.hand_s[0]);
      auto res1 = use_win(bfp, bfp.hand_s[1]);
      if(res0.first && res1.first) {
        if(res0.second < res1.second) {
          res.first = bfp.hand_s[0];
          res.second = res0.second;
        } else {
          res.first = bfp.hand_s[1];
          res.second = res1.second;
        }
      } else if(res0.first) {
        res.first = bfp.hand_s[0];
        res.second = res0.second;
      } else if(res1.first) {
        res.first = bfp.hand_s[1];
        res.second = res1.second;
      } else {
        res.first = 0;
        res.second = 0;
      }
    }
    return res;
  }
  // 相手ターンの場合
  else
    return {0, 0};
}

std::pair<bool, int> use_win(const bf_position& bfp, int card) {
  auto t = is_terminated_win(bfp);
  if(t.first != -1) return t;

  if(card == 3 && !bfp.barrier_s && bfp.hand_e_min() > bfp.other_hand_s(3)) {
    return {false, 0};
  }
  if(card == 8) return {false, 0};
  if(commentablebfp) cout << bfp.count_deck() << "use :" << card << endl;

  struct bf_position next_bfp = bfp;
  next_bfp.is_my_turn = !bfp.is_my_turn;
  next_bfp.trash[card - 1] += 1; //公開する
  //手札を減らす
  if(bfp.hand_s[0] == card) {
    next_bfp.hand_s[0] = bfp.hand_s[1];
    next_bfp.hand_s[1] = 0;
  } else if(bfp.hand_s[1] == card) {
    next_bfp.hand_s[1] = 0;
  } else {
    cerr << "Error: card not in hand" << endl;
    exit(2);
  }

  next_bfp = reset_flag_by_use(next_bfp, true, card);

  // if(card == 5 || (card == 6 && bfp.barrier_e)) next_bfp.not7_flag_s = true;//自分のフラグは不要

  if(bfp.barrier_e && card != 4 && card != 5 && card != 7) {
    auto res = enemy_turn_win(next_bfp);
    return {res.first, res.first ? res.second + 1 : 0};
  } else if(card == 1) {
    next_bfp.is_sol_choice = true;
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bfp, "sol");
    for(int i = 1; i < 8; i++) {
      if(bfp.hand_e(i)) {
        auto res = sol_win(next_bfp, i + 1);
        if(res.first) {
          has_true = true;
          min_t = std::min(min_t, res.second);
        }
      }
    }
    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == 2) {
    bool all_true = true;
    int max_f = -1;

    for(int i = 0; i < 8; i++) {
      if(all_true)
        if(bfp.hand_e(i)) {
          struct bf_position next_bfp2 = next_bfp;
          next_bfp2.open_flag_e = i + 1;
          auto res = enemy_turn_win(next_bfp2);
          if(res.first) {
            max_f = std::max(max_f, res.second);
          } else {
            all_true = false;
          }
        }
    }
    if(max_f == -1) return {false, 0};
    return {all_true, all_true ? max_f + 1 : 0};
  } else if(card == 3) {
    int other = bfp.other_hand_s(3);
    if(bfp.hand_e(other - 1)) {
      struct bf_position next_bfp2 = next_bfp;
      next_bfp2.open_flag_e = other;
      // next_bfp2.open_flag_s = other;//自分のフラグは不要
      auto res = enemy_turn_win(next_bfp2);
      return {res.first, res.first ? res.second + 1 : 0};
    }
    return {false, 0};
  } else if(card == 4) {
    next_bfp.barrier_s = true;
    auto res = enemy_turn_win(next_bfp);
    return {res.first, res.first ? res.second + 1 : 0};
  } else if(card == 5) {
    next_bfp.is_wiz_choice = true;
    auto res_self = wiz_win(next_bfp, true);
    auto res_enemy = wiz_win(next_bfp, false);

    bool has_true = res_self.first || res_enemy.first;

    int min_t = 1e9;
    if(res_self.first) min_t = std::min(min_t, res_self.second);
    if(res_enemy.first) min_t = std::min(min_t, res_enemy.second);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == 6) {
    next_bfp.reset_flag(true);
    next_bfp.reset_flag(false);
    next_bfp.open_flag_e = bfp.other_hand_s(card);
    bool all_true = true;
    int max_f = -1;

    for(int i = 0; i < 8; i++) {
      if(bfp.hand_e(i)) {
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.hand_s[0] = i + 1;
        // next_bfp2.open_flag_s = i + 1;//自分のフラグは不要
        auto res = enemy_turn_win(next_bfp2);
        if(res.first) {
          max_f = std::max(max_f, res.second);
        } else {
          all_true = false;
        }
      }
    }
    if(max_f == -1) return {false, 0};
    return {all_true, all_true ? max_f + 1 : 0};
  } else if(card == 7) {
    // next_bfp.lt5_flag_s = true;//自分のフラグは不要
    auto res = enemy_turn_win(next_bfp);
    return {res.first, res.first ? res.second + 1 : 0};
  } else return {false, 0};
}

std::pair<bool, int> enemy_turn_win(const bf_position& bfp) {
  auto t = is_terminated_win(bfp);
  if(t.first != -1) return t;

  bool all_true = true;
  int max_f = -1;

  for(int i = 0; i < 7; i++) {
    if(!all_true) break;
    if(bfp.deck_or_hand_e(i) > 0) {
      if(commentablebfp) cout << bfp.count_deck() - 1 << "enemy :" << i + 1 << endl;
      // 1. カード効果による即時敗北（深さ0の敗北として扱う）
      if(i + 1 == 1 && !bfp.barrier_s && bfp.hand_s[0] > 1) {
        all_true = false;
        continue;
      }

      if(i + 1 == 3 && !bfp.barrier_s) {
        bool immediate_loss = false;
        for(int j = 0; j < 8; j++) {
          if(bfp.deck_or_hand_e(j)) {
            if(j + 1 == 3 && bfp.deck_or_hand_e(2) >= 2 && bfp.hand_s[0] < 3) {
              immediate_loss = true;
              break;
            } else if(j + 1 > bfp.hand_s[0]) {
              immediate_loss = true;
              break;
            }
          }
        }
        if(immediate_loss) {
          all_true = false;
          continue;
        }
      }

      if(i + 1 == 5 && bfp.hand_s[0] == 8 && !bfp.barrier_s) {
        all_true = false;
        continue;
      }

      // 2. 状態の更新
      struct bf_position next_bfp = bfp;
      next_bfp.trash[i] += 1;
      next_bfp.barrier_e = false;
      next_bfp.is_my_turn = !bfp.is_my_turn;
      next_bfp = reset_flag_by_use(next_bfp, false, i + 1);

      if(i + 1 == 5) next_bfp.not7_flag_e = true;

      // 3. 各カードごとの再帰評価
      if(i + 1 == 3) {
        if(!bfp.barrier_s) next_bfp.open_flag_e = bfp.hand_s[0];
        auto res = draw_win(next_bfp);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else if(i + 1 == 4) {
        next_bfp.barrier_e = true;
        auto res = draw_win(next_bfp);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else if(i + 1 == 5) {
        if(bfp.open_e() == 7) continue;

        // 魔術師専用の集約ラムダ
        auto eval_wiz_preds = [&](const std::vector<bf_position>& preds) -> std::pair<bool, int> {
          if(preds.empty()) return {false, 0}; // 空なら敗北(深さ0)扱い
          int local_max_f = -1;
          bool local_all_true = true;
          for(const auto& p : preds) {
            auto res = draw_win(p);
            if(res.first) local_max_f = std::max(local_max_f, res.second);
            else local_all_true = false;
          }
          return {local_all_true, local_all_true ? local_max_f : 0};
        };

        std::vector<bf_position> preds_toself = ef_wizard(next_bfp, true);
        auto res_self = eval_wiz_preds(preds_toself);

        std::vector<bf_position> preds_toenemy = ef_wizard(next_bfp, false);
        auto res_enemy = eval_wiz_preds(preds_toenemy);

        if(res_self.first && res_enemy.first) max_f = std::max(res_self.second, res_enemy.second);
        else all_true = false;
      } else if(i + 1 == 6) {
        if(bfp.open_e() == 7) continue;
        if(!bfp.barrier_s) {
          bool gene_all_true = true;
          int gene_max_f = -1;

          for(int j = 0; j < 8; j++) {
            if(!gene_all_true) break;
            if(next_bfp.hand_e(j)) {
              struct bf_position next_bfp2 = next_bfp;
              next_bfp2.hand_s[0] = j + 1;
              next_bfp2.reset_flag(true);
              next_bfp2.reset_flag(false);
              next_bfp2.open_flag_e = bfp.hand_s[0];
              auto res = draw_win(next_bfp2);
              if(res.first) gene_max_f = std::max(gene_max_f, res.second);
              else gene_all_true = false;
            }
          }
          if(gene_all_true && gene_max_f != -1) max_f = std::max(max_f, gene_max_f);
          else all_true = false;
        } else {
          next_bfp.not7_flag_e = true;
          auto res = draw_win(next_bfp);
          if(res.first) max_f = std::max(max_f, res.second);
          else all_true = false;
        }
      } else if(i + 1 == 7) {
        next_bfp.lt5_flag_e = true;

        // 7 を使った相手は、使用後も 5 未満のカードを手札に残している必要がある。
        // deck_or_hand_e() に 7 自身が存在するだけでは、この枝は成立しない。
        bool has_remaining_hand_candidate = false;
        for(int j = 0; j < 8; j++) {
          if(next_bfp.hand_e(j)) {
            has_remaining_hand_candidate = true;
            break;
          }
        }
        if(!has_remaining_hand_candidate) continue;

        auto res = draw_win(next_bfp);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else {
        auto res = draw_win(next_bfp);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      }
    }
  }

  // 指定された win に応じて最適な深さを +1 して返す
  return {all_true, all_true ? max_f + 1 : 0};
}

std::pair<bool, int> draw_win(const bf_position& bfp) {
  auto t = is_terminated_win(bfp);
  if(t.first != -1) return t;

  bool all_true = true;
  int max_f = -1;

  for(int i = 0; i < 8; i++) {
    if(!all_true) break;
    if(bfp.deck(i)) {
      bf_position next_bfp = draw(bfp, i + 1);
      next_bfp.barrier_s = false;
      next_bfp.is_my_turn = !next_bfp.is_my_turn;
      if(commentablebfp) cout << next_bfp.count_deck() << "draw " << i + 1 << endl;

      // --- 自分の手札の選択 (ORノード) ---
      auto res0 = use_win(next_bfp, next_bfp.hand_s[0]);

      bool or_first = false;
      int or_second = 0;

      // 手札の2枚が違うカードなら、もう一方も評価する
      if(next_bfp.hand_s[0] != i + 1) {
        auto res1 = use_win(next_bfp, next_bfp.hand_s[1]);

        if(res0.first) {
          or_first = true;
          or_second = res0.second;
        }
        if(res1.first) {
          if(!or_first) or_second = res1.second;
          else or_second = std::min(or_second, res1.second);
          or_first = true;
        }
      } else {
        // 同じカードなら片方の結果をそのまま使う
        or_first = res0.first;
        or_second = res0.second;
      }

      // --- 山札ドローの集約 (ANDノード) ---
      if(or_first) {
        max_f = std::max(max_f, or_second);
      } else {
        all_true = false;
      }
    }
  }

  if(max_f == -1 || !all_true) return {false, 0};

  return {all_true, all_true ? max_f : 0};
}

bf_position draw(const bf_position& bfp, int draw_card) {
  assert(bfp.deck(draw_card - 1) && bfp.count_deck() > 0 && bfp.hand_s[1] == 0);
  bf_position next_bfp = bfp;
  next_bfp.hand_s[1] = draw_card;
  return next_bfp;
}

std::pair<bool, int> sol_win(const bf_position& bfp, int card) {
  if(!bfp.is_sol_choice) exit_with_print(bfp, "sol_win called when not in sol_choice state");
  if(bfp.open_e() == card && !bfp.barrier_e) return {true, 1};
  struct bf_position next_bfp = bfp;
  // 宣言が確定したので、保留中の兵士選択を解除する。
  next_bfp.is_sol_choice = false;
  next_bfp.add_sol_e(card);
  auto res = enemy_turn_win(next_bfp);
  return {res.first, res.first ? res.second + 1 : 0};
}

std::pair<bool, int> wiz_win(const bf_position& bfp, bool to0p) {
  if(!bfp.is_wiz_choice) exit_with_print(bfp, "wiz_win called when not in wiz_choice state");
  if(!to0p && bfp.open_e() == 8 && !bfp.barrier_e) return {true, 1};
  if(to0p && bfp.hand_s[0] == 7) return {false, 0};
  struct bf_position next_bfp = bfp;
  next_bfp.is_wiz_choice = false;
  std::vector<bf_position> preds = ef_wizard(next_bfp, to0p);
  if(preds.empty()) return {false, 0};

  int local_max_f = -1;
  bool local_all_true = true;
  for(const auto& p : preds) {
    auto res = enemy_turn_win(p);
    if(res.first) {
      local_max_f = std::max(local_max_f, res.second);
    } else {
      local_all_true = false;
    }
  }
  return {local_all_true, local_all_true ? local_max_f + 1 : 0};
}

std::vector<bf_position> ef_wizard(const bf_position& bfp, bool to_0p) {
  std::vector<bf_position> bfps;
  if(bfp.is_my_turn == false) { // use_abswinの最初でturnを切り替えるためturn==falseは0playerのターン
    if(to_0p) {
      if(bfp.hand_s[0] == 8) {
        // return false;
        return bfps;
      }
      for(int i = 0; i < 8; i++) {
        if(bfp.deck(i)) {
          bf_position next_bfp = bfp;
          next_bfp.is_wiz_choice = false;
          next_bfp.is_my_turn = !bfp.is_my_turn;
          next_bfp.trash[bfp.hand_s[0] - 1] += 1; //手札捨てる
          next_bfp.hand_s[0] = i + 1; //手札引く
          next_bfp.reset_flag(true); //自分のフラグリセット
          bfps.push_back(next_bfp);
        }
      }
      return bfps;
    } else {
      if(bfp.barrier_e) {
        bf_position next_bfp = bfp;
        next_bfp.is_wiz_choice = false;
        bfps.push_back(next_bfp);
        return bfps;
      }
      // validate_hand_e_candidate(bfp, "wiz");
      for(int i = 0; i < 8; i++) {
        if(bfp.hand_e(i) && i + 1 != 8) {
          bf_position next_bfp = bfp;
          next_bfp.is_wiz_choice = false;
          next_bfp.trash[i] += 1;
          next_bfp.is_my_turn = !bfp.is_my_turn;
          next_bfp.reset_flag(false); //相手のフラグリセット
          bfps.push_back(next_bfp);
        }
      }
      return bfps;
    }
  } else { // is_my_turn == true
    if(to_0p) {
      if(bfp.barrier_s) {
        bf_position next_bfp = bfp;
        next_bfp.is_wiz_choice = false;
        bfps.push_back(next_bfp);
        return bfps;
      }
      for(int i = 0; i < 8; i++) {
        if(bfp.deck(i)) {
          bf_position next_bfp = bfp;
          next_bfp.is_wiz_choice = false;
          next_bfp.is_my_turn = !bfp.is_my_turn;
          next_bfp.trash[bfp.hand_s[0] - 1] += 1; //手札捨てる
          next_bfp.hand_s[0] = i + 1; //手札引く
          next_bfp.reset_flag(true); //自分のフラグリセット
          bfps.push_back(next_bfp);
        }
      }
      return bfps;
    } else {
      if(bfp.open_e() == 8) {
        return bfps;
      }
      // validate_hand_e_candidate(bfp, "wiz");
      for(int i = 0; i < 8; i++) {
        if(bfp.hand_e(i) && i + 1 != 8) {
          bf_position next_bfp = bfp;
          next_bfp.is_wiz_choice = false;
          next_bfp.is_my_turn = !bfp.is_my_turn;
          next_bfp.trash[i] += 1;
          next_bfp.reset_flag(false); //相手のフラグリセット
          bfps.push_back(next_bfp);
        }
      }
      return bfps;
    }
  }
}

void bf_position::print() const {
  // cout << "depth : " << depth << endl;
  cout << "barrier_s : " << barrier_s << " barrier_e : " << barrier_e << " is_my_turn : " << is_my_turn << endl;
  cout << "is_sol_choice : " << is_sol_choice << " is_wiz_choice : " << is_wiz_choice << " deck : " << count_deck() << endl;
  cout << "open_flag_e : " << open_flag_e << " sol_flag_e : " << sol_flag_e[0] << " " << sol_flag_e[1] << " lt5_flag_e : " << lt5_flag_e << " not7_flag_e : " << not7_flag_e << endl;
  cout << "open_flag_s : " << open_flag_s << " sol_flag_s : " << sol_flag_s[0] << " " << sol_flag_s[1] << " lt5_flag_s : " << lt5_flag_s << " not7_flag_s : " << not7_flag_s << endl;
  cout << "hand_s : " << hand_s[0] << " " << hand_s[1] << " ";
  cout << "trash:";
  for(int i = 0; i < 8; i++) {
    cout << trash[i] << " ";
  }
  cout << " deck_or_hand_e : ";
  for(int i = 0; i < 8; i++) {
    cout << deck_or_hand_e(i) << " ";
  }
  cout << endl;
  cout << "hand_e: ";
  for(int i = 0; i < 8; i++) {
    cout << hand_e(i) << " ";
  }
  cout << endl
       << endl;
}

[[noreturn]] void exit_with_print(const bf_position& bfp, const char* context) {
  bfp.print();
  cerr << "Error: " << context << endl;
  exit(2);
}

void validate_hand_e_candidate(const bf_position& bfp, const char* context) {
  for(int i = 0; i < 8; i++) {
    if(bfp.hand_e(i)) return;
  }
  exit_with_print(bfp, (std::string(context) + " : hand_e candidate is empty").c_str());
}

unsigned char action2char(int x, bool rnd) {
  int act1, act2;
  if(x > 10000) {
    act1 = x / 1000;
    act2 = x % 1000;
  } else if(x > 1000) {
    act1 = x / 100;
    act2 = x % 100;
  } else if(x > 100) {
    act1 = x / 10;
    act2 = x % 10;
  } else {
    act1 = x;
    act2 = 0;
  }

  char action[2];
  size_t s = 1;
  action[0] = action_to_char(act1 / 10, act1 % 10);

  if(act2 > 0) {
    if(act1 % 10 == 4) {
      action[1] = wizard_to_char(act2 / 100, (act2 / 10) % 10, act2 % 10);
    } else if(act1 % 10 == 2) {
      action[1] = twonum_to_char(act2 % 10, act2 % 10);
    } else {
      action[1] = twonum_to_char(act2 / 10, act2 % 10);
    }
    s = 2;
  }

  unsigned int h;
  if(rnd) {
    h = rph.get_hash(action, s);
  } else {
    h = oph.get_hash(action, s);
  }
  return static_cast<unsigned char>(h);
}

std::string actions_to_string(const std::vector<int>& actions, bool rnd) {
  std::string history;
  history.reserve(actions.size());

  for(auto x : actions) {
    history.push_back(action2char(x, rnd));
  }
  return history;
}

string get_actions_history(std::string s, bool rnd = true) {
  long unsigned int head = 0;
  int turn = 0;
  bool bal[2] = {false, false};
  string history = "";
  while(head < s.size()) {
    string action;
    if(rnd) {
      action = rph.get_action((unsigned char)s[head]);
    } else {
      action = oph.get_action((unsigned char)s[head]);
    }
    int c2a = char_to_action(action[0]);
    head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;
    history += to_string(c2a);
    // cout << c2a;
    bal[turn] = false;
    if(num1 == 4) {
      if(num2 == 0 && !bal[1 - turn] && action.size() > 1) {
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t % 10);
      } else if(num2 == 1 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t % 10);
        // cout << (c2t % 10);
      } else if(num2 == 2 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t % 10);
        // cout << (c2t % 10);
      } else if(num2 == 3) {
        // 次の行動で turn が反転するため bal[1 - turn] として読まれるが,
        // cppcheck は添字の変化を追えず未使用と誤検知する
        // cppcheck-suppress unreadVariable
        bal[turn] = true;
      } else if(num2 == 4) {
        int c2w = char_to_wizard(action[1]);
        history += to_string(c2w / 100);
        history += to_string((c2w / 10) % 10);
        history += to_string(c2w % 10);
        // cout << c2w / 100 << ((c2w / 10) % 10) << (c2w % 10);
        // cout << c2w;
      } else if(num2 == 5 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t / 10);
        history += to_string(c2t % 10);
        // cout << (c2t / 10) << (c2t % 10);
      }
      turn = !turn;
    }
    // cout << " ";
    history += " ";
  }
  // cout << endl;
  return history;
}

void output_actions_history(std::string s, bool rnd = true) {
  cout << get_actions_history(s, rnd) << endl;
}

bf_position swap_player(const bf_position& bfp, const int hand) {
  bf_position next_bfp = bfp;
  next_bfp.is_my_turn = !bfp.is_my_turn;
  std::swap(next_bfp.barrier_s, next_bfp.barrier_e);
  std::swap(next_bfp.open_flag_e, next_bfp.open_flag_s);
  std::swap(next_bfp.sol_flag_e, next_bfp.sol_flag_s);
  std::swap(next_bfp.lt5_flag_e, next_bfp.lt5_flag_s);
  std::swap(next_bfp.not7_flag_e, next_bfp.not7_flag_s);
  next_bfp.hand_s[0] = hand;
  next_bfp.hand_s[1] = 0;
  return next_bfp;
}

//<使うカード, 敗北するまでのターン数>を返す。敗北しない場合は<0, 0>
//ルール上すでに敗北の場合9, 魔術師の使用による敗北で,対象自分のみなら15,対象相手のみなら25
std::vector<std::pair<int, int>> is_lose(const bf_position& bfp) {

  if(bfp.have_s(7) && bfp.hand_s[0] + bfp.hand_s[1] >= 12) {
    return {{9, 0}};
  }
  if(bfp.count_deck() < 2 && bfp.hand_s[1] == 0 && !bfp.is_wiz_choice && !bfp.is_sol_choice) {
    int min = bfp.hand_e_min();
    if(min > bfp.hand_s[0]) return {{9, 0}};
    else return {};
  }
  if(bfp.have_s(8)) return {{8, 1}};

  std::vector<std::pair<int, int>> res;

  if(bfp.hand_s[1] == 0 && bfp.is_sol_choice) return {};
  else if(bfp.hand_s[1] == 0 && bfp.is_wiz_choice) {
    auto res_self = wiz_lose(bfp, true);
    auto res_enemy = wiz_lose(bfp, false);

    if(res_self.first) res.push_back({0, res_self.second});
    if(res_enemy.first) res.push_back({1, res_enemy.second});
    return res;
  } else if(bfp.is_my_turn && bfp.hand_s[1] != 0) {
    auto res0 = use_lose(bfp, bfp.hand_s[0]);
    auto res1 = use_lose(bfp, bfp.hand_s[1]);
    if(res0.first) res.push_back({bfp.hand_s[0], res0.second});
    if(res1.first) res.push_back({bfp.hand_s[1], res1.second});
    return res;
  } else return {};
}

std::pair<bool, int> use_lose(const bf_position& bfp, int card) {
  if(card == 3 && !bfp.barrier_e) {
    if(bfp.hand_e_min() > bfp.other_hand_s(3)) return {true, 1};
    if(bfp.hand_e_min() < bfp.other_hand_s(3)) return {false, 0};
  }
  struct bf_position next_bfp = bfp;
  next_bfp.is_my_turn = false;
  next_bfp.trash[card - 1] += 1; // 公開する

  // 手札を減らす
  if(bfp.hand_s[0] == card) {
    next_bfp.hand_s[0] = bfp.hand_s[1];
    next_bfp.hand_s[1] = 0;
  } else if(bfp.hand_s[1] == card) {
    next_bfp.hand_s[1] = 0;
  } else {
    cerr << "Error: card not in hand" << endl;
    exit(2);
  }

  next_bfp = reset_flag_by_use(next_bfp, true, card);

  if(card >= 5) next_bfp.not7_flag_s = true;

  // ANDノードの深さ集約用変数
  int max_f = -1;

  if(bfp.barrier_e && card != 4 && card != 5 && card != 7) {
    for(int i = 0; i < 8; i++) {
      if(next_bfp.hand_e(i)) {
        struct bf_position next_bfp2 = swap_player(next_bfp, i + 1);
        auto res = draw_win(next_bfp2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 1) {
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bfp, "sol");
    for(int i = 1; i < 8; i++) {
      if(bfp.hand_e(i)) return {false, 0};
    }
    if(bfp.hand_e(0)) {
      struct bf_position next_bfp2 = swap_player(next_bfp, 1);
      auto res = draw_win(next_bfp2);
      //ランダム宣言のみ
      return {res.first, res.first ? res.second + 1 : 0};
    }
  } else if(card == 2) {
    for(int i = 0; i < 8; i++) {
      if(bfp.hand_e(i)) {
        struct bf_position next_bfp2 = swap_player(next_bfp, i + 1);
        // next_bfp2.open_flag_s = i + 1;//自分のフラグは不要
        auto res = draw_win(next_bfp2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 3) {
    int other = bfp.other_hand_s(3);
    if(bfp.hand_e(other - 1)) {
      struct bf_position next_bfp2 = swap_player(next_bfp, other);
      next_bfp2.open_flag_e = other;
      // next_bfp2.open_flag_s = other;//自分のフラグは不要
      auto res = draw_win(next_bfp2);
      if(res.first) return {true, res.second + 1};
    }
    return {false, 0};
  } else if(card == 4) {
    next_bfp.barrier_s = true;
    for(int i = 0; i < 8; i++) {
      if(next_bfp.hand_e(i)) {
        struct bf_position next_bfp2 = swap_player(next_bfp, i + 1);
        auto res = draw_win(next_bfp2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 5) {
    auto res_self = wiz_lose(next_bfp, true);
    auto res_enemy = wiz_lose(next_bfp, false);

    bool has_true = res_self.first && res_enemy.first;

    int min_t = std::min(res_self.second, res_enemy.second);
    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == 6) {
    int other = bfp.other_hand_s(6);
    next_bfp.open_flag_e = other;
    for(int i = 0; i < 8; i++) {
      if(bfp.hand_e(i)) {
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.hand_s[0] = i + 1;
        next_bfp2.open_flag_s = i + 1;
        struct bf_position next_bfp3 = swap_player(next_bfp2, other);
        auto res = draw_win(next_bfp3);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 7) {
    next_bfp.lt5_flag_s = true;
    for(int i = 0; i < 8; i++) {
      if(next_bfp.hand_e(i)) {
        struct bf_position next_bfp2 = swap_player(next_bfp, i + 1);
        auto res = draw_win(next_bfp2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    if(max_f == -1) return {false, 0};
    return {true, max_f + 1};
  }
  return {false, 0};
}

std::pair<bool, int> wiz_lose(const bf_position& bfp, bool to0p) {
  if(!to0p && bfp.hand_e(7) && !bfp.barrier_e) return {false, 0};
  if(to0p && bfp.hand_s[0] == 7) return {true, 0};
  std::vector<bf_position> preds = ef_wizard(bfp, to0p);
  if(preds.empty()) return {false, 0};
  int local_max_f = -1;
  for(const auto& p : preds) {
    validate_hand_e_candidate(p, "wizlose");
    for(int i = 0; i < 8; i++) {
      if(p.hand_e(i)) {
        struct bf_position next_bfp = swap_player(p, i + 1);
        auto res = draw_win(next_bfp);
        if(res.first) local_max_f = std::max(local_max_f, res.second);
        else return {false, 0};
      }
    }
  }
  return {true, local_max_f};
}

std::vector<int> able_actions(const bf_position& bfp, int card, bool is_second_player) {
  int base = 40 + card - 1;
  std::vector<int> actions;

  if(bfp.hand_s[1] == 0 && bfp.is_wiz_choice) {
    // --- 自分を対象とする場合 ---
    if((card == 0 && !is_second_player) || (card == 1 && is_second_player)) {
      for(int j = 0; j < 8; j++)
        if(bfp.deck(j)) actions.push_back(44000 + is_second_player * 100 + (bfp.other_hand_s(card) - 1) * 10 + j);
    }
    // --- 相手を対象とする場合 ---
    else if((card == 0 && is_second_player) || (card == 1 && !is_second_player)) {
      if(!bfp.barrier_e) {
        for(int i = 0; i < 7; i++) {
          if(bfp.hand_e(i)) { // 相手が捨てさせられるカード
            actions.push_back(44000 + !is_second_player * 100 + i * 10 + 0);
          }
        }
      } else actions.push_back(44000 + !is_second_player * 100);
    }
  } else if(card == 4 || card == 7 || bfp.barrier_e || card == 1) {
    actions.push_back(base);
  } else if(card == 2) {
    // 相手の判明するカード
    for(int i = 0; i < 8; i++)
      if(bfp.hand_e(i)) actions.push_back(base * 10 + i);
  } else if(card == 3) {
    // 相手の判明するカード
    int other_i = bfp.other_hand_s(card) - 1;
    if(bfp.hand_e(other_i)) actions.push_back(base * 100 + other_i * 11);
  } else if(card == 6) {
    // 相手と交換するカード
    for(int i = 0; i < 8; i++)
      if(bfp.hand_e(i)) actions.push_back(base * 100 + (bfp.other_hand_s(card) - 1) * 10 + i);
  }

  return actions;
}

int action_count(const bf_position& bfp) {
  if(bfp.is_sol_choice) {
    int count = 0;
    for(int i = 1; i < 8; i++) {
      if(bfp.deck_or_hand_e(i)) count++;
    }
    return count;
  }
  if(bfp.is_wiz_choice) return 2;

  if(bfp.hand_s[1] == 0) return 0;
  return bfp.hand_s[0] == bfp.hand_s[1] ? 1 : 2;
}

bool is_openhand_loveletter(const bf_position& bfp) {
  return bfp.open_e() > 1 && bfp.open_s() > 1 && bfp.hand_s[1] == 0;
}

bool is_openhand_loveletter_hisp(int open[3], std::string history, bool rnd) {
  bf_position bfp(open, history, rnd);
  return is_openhand_loveletter(bfp);
}

bool is_openhand_loveletter_his(int open[3], std::string his0, std::string his1, bool rnd) {
  bf_position bfp0(open, his0, rnd);
  bf_position bfp1(open, his1, rnd);
  return bfp0.open_e() > 1 && bfp1.open_e() > 1 && bfp0.hand_s[1] == 0 && bfp1.hand_s[1] == 0;
}
#endif
