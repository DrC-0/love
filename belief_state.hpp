#ifndef BELIEF_STATE_HPP
#define BELIEF_STATE_HPP
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <cassert>

#include "action_code.hpp"
#include "card_table.hpp"
#include "count_work.hpp"

inline bool commentable_bs = false;

struct belief_state {
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
  belief_state()
    : is_my_turn(false), is_sol_choice(false), is_wiz_choice(false), not7_flag_s(false), not7_flag_e(false),
      barrier_s(false), barrier_e(false), lt5_flag_s(false), lt5_flag_e(false), open_flag_s(0), open_flag_e(0),
      sol_flag_s{0, 0}, sol_flag_e{0, 0}, hand_s{0, 0}, trash{0, 0, 0, 0, 0, 0, 0, 0} {}
  belief_state(bool is_my_turn, bool is_sol_choice, bool is_wiz_choice, bool not7_flag_s, bool not7_flag_e, bool barrier_s, bool barrier_e,
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
  belief_state(int open[3], std::string history, bool rnd);
  int deck_or_hand_e(int card) const;
  bool hand_e(int card) const;
  bool hand_s_est(int card) const;
  int open_e() const;
  int open_s() const;
  bool deck(int card) const;
  // open_e() は8要素ループの中で不変なので、括り出した値を渡せる版。
  bool deck(int card, int open_card) const;
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

int trash_and_hand_s(const int card, const int hand[2], const int trash[8]);
int deck_or_hand_e(const int card, const int hand[2], const int trash[8]);
bool hand_e(const int card, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e);

belief_state reset_flag_by_use(const belief_state& bs, bool is_self, int card);
// ef_wizard の候補置き場。呼び出し側は1回舐めて捨てるだけなので、
// std::vector を作らず呼び出し側のスタックに置く。要素は
// for(int i = 0; i < 8; i++) の中で高々1回ずつ push されるので最大8個。
struct ef_wizard_preds {
  belief_state items[8];
  int n = 0;
  void push(const belief_state& b) {
    assert(n < 8);
    items[n++] = b;
  }
  bool empty() const {
    return n == 0;
  }
  const belief_state* begin() const {
    return items;
  }
  const belief_state* end() const {
    return items + n;
  }
};
void ef_wizard(const belief_state& bs, bool to_0p, ef_wizard_preds& out);
belief_state draw(const belief_state& bs, int draw_card);
belief_state swap_player(const belief_state& bs, const int hand);
[[noreturn]] void exit_with_print(const belief_state& bs, const char* context);
void validate_hand_e_candidate(const belief_state& bs, const char* context);

int trash_and_hand_s(const int card, const int hand[2], const int trash[8]) {
  return trash[card - 1] + (hand[0] == card ? 1 : 0) + (hand[1] == card ? 1 : 0);
}

int deck_or_hand_e(const int card, const int hand[2], const int trash[8]) {
  CW_BUMP(deck_or_hand_e);
  return max_num[card - 1] - trash_and_hand_s(card, hand, trash);
}

bool hand_e(const int card, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e) {
  CW_BUMP(hand_e);
  if(open_flag_e > 0) { //手札が確定している場合
    if(open_flag_e == card) {
      return true;
    } else {
      return false;
    }
  } else if(sol_flag_e[0] > 1 && sol_flag_e[0] == card) {
    return false;
  } else if(sol_flag_e[1] > 1 && sol_flag_e[1] == card) {
    return false;
  } else if(lt5_flag_e && card >= 5) { //前のターンに7を出したときの,5以上
    return false;
  } else if(not7_flag_e && card == 7) { //前のターンに5を出したときの,7
    return false;
  } else {
    return deck_or_hand_e(card, hand, trash) > 0;
  }
}

int belief_state::deck_or_hand_e(int card) const {
  return ::deck_or_hand_e(card, hand_s, trash);
}

bool belief_state::hand_e(int card) const {
  return ::hand_e(card, hand_s, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag_e);
}

bool belief_state::hand_s_est(int card) const {
  int enemy_hand[2] = {open_e(), 0};
  return ::hand_e(card, enemy_hand, trash, open_flag_s, sol_flag_s, lt5_flag_s, not7_flag_s);
}

int belief_state::open_e() const {
  CW_BUMP(open_e);
  int found = 0;
  for(int card = 1; card <= 8; card++) {
    if(hand_e(card)) {
      if(found == 0) {
        found = card;
      } else {
        return 0;
      }
    }
  }
  return found;
}

int belief_state::open_s() const {
  int found = 0;
  for(int card = 1; card <= 8; card++) {
    if(hand_s_est(card)) {
      if(found == 0) {
        found = card;
      } else {
        return 0;
      }
    }
  }
  return found;
}

bool belief_state::deck(int card, int open_card) const {
  CW_BUMP(deck);
  return deck_or_hand_e(card) > (card == open_card ? 1 : 0);
}

bool belief_state::deck(int card) const {
  return deck(card, open_e());
}

bool belief_state::have_s(int card) const {
  return hand_s[0] == card || hand_s[1] == card;
}

int belief_state::other_hand_s(int card) const {
  if(hand_s[0] == card) {
    return hand_s[1];
  } else if(hand_s[1] == card) {
    return hand_s[0];
  } else {
    return 0;
  }
}

int belief_state::count_deck() const {
  CW_BUMP(count_deck);
  int count = 0;
  for(int card = 1; card <= 8; card++) {
    count += deck_or_hand_e(card);
  }
  return count - 1;
}

int belief_state::hand_e_max() const {
  CW_BUMP(hand_e_max);
  int max_card = 0;
  for(int card = 1; card <= 8; card++) {
    if(hand_e(card)) {
      max_card = card;
    }
  }
  return max_card;
}

int belief_state::hand_e_min() const {
  CW_BUMP(hand_e_min);
  int min_card = 0;
  for(int card = 1; card <= 8; card++) {
    if(hand_e(card)) {
      if(min_card == 0 || card < min_card) {
        min_card = card;
      }
    }
  }
  return min_card;
}

int belief_state::deck_or_hand_e_min() const {
  CW_BUMP(deck_or_hand_e_min);
  int min_card = 0;
  for(int card = 1; card <= 8; card++) {
    if(deck_or_hand_e(card) > 0) {
      if(min_card == 0 || card < min_card) {
        min_card = card;
      }
    }
  }
  return min_card;
}

void belief_state::add_sol_s(int card) {
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

void belief_state::add_sol_e(int card) {
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

void belief_state::reset_flag(bool is_self) {
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

belief_state reset_flag_by_use(const belief_state& bs, bool to_self, int card) {
  CW_BUMP(reset_flag_by_use);
  struct belief_state next_bs = bs;
  if(to_self) {
    if(bs.open_flag_s > 0 && bs.open_flag_s == card) {
      next_bs.open_flag_s = 0;
    }
    if(bs.sol_flag_s[1] != 0 && bs.sol_flag_s[1] != card) {
      next_bs.sol_flag_s[1] = 0;
    }
    if(bs.sol_flag_s[0] != 0 && bs.sol_flag_s[0] != card) {
      next_bs.sol_flag_s[0] = next_bs.sol_flag_s[1];
      next_bs.sol_flag_s[1] = 0;
    }
    if(bs.lt5_flag_s && card < 5) {
      next_bs.lt5_flag_s = false;
    }
    next_bs.not7_flag_s = false; //対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

  } else {
    if(bs.open_flag_e > 0 && bs.open_flag_e == card) {
      next_bs.open_flag_e = 0;
    }
    if(bs.sol_flag_e[1] != 0 && bs.sol_flag_e[1] != card) {
      next_bs.sol_flag_e[1] = 0;
    }
    if(bs.sol_flag_e[0] != 0 && bs.sol_flag_e[0] != card) {
      next_bs.sol_flag_e[0] = next_bs.sol_flag_e[1];
      next_bs.sol_flag_e[1] = 0;
    }
    if(bs.lt5_flag_e && card < 5) {
      next_bs.lt5_flag_e = false;
    }
    next_bs.not7_flag_e = false;
  }
  return next_bs;
}

belief_state draw(const belief_state& bs, int draw_card) {
  assert(bs.deck(draw_card) && bs.count_deck() > 0 && bs.hand_s[1] == 0);
  belief_state next_bs = bs;
  next_bs.hand_s[1] = draw_card;
  return next_bs;
}

void ef_wizard(const belief_state& bs, bool to_0p, ef_wizard_preds& out) {
  CW_BUMP(ef_wizard);
  if(bs.is_my_turn == false) { // use_abswinの最初でturnを切り替えるためturn==falseは0playerのターン
    if(to_0p) {
      if(bs.hand_s[0] == 8) {
        // return false;
        return;
      }
      // open_e() はこのループの中で不変 (bs は const 参照) なので括り出す。
      const int open_card = bs.open_e();
      for(int card = 1; card <= 8; card++) {
        if(bs.deck(card, open_card)) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.is_my_turn = !bs.is_my_turn;
          next_bs.trash[bs.hand_s[0] - 1] += 1; //手札捨てる
          next_bs.hand_s[0] = card; //手札引く
          next_bs.reset_flag(true); //自分のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    } else {
      if(bs.barrier_e) {
        belief_state next_bs = bs;
        next_bs.is_wiz_choice = false;
        CW_BUMP(ef_wizard_elem);
        out.push(next_bs);
        return;
      }
      // validate_hand_e_candidate(bs, "wiz");
      for(int card = 1; card <= 8; card++) {
        if(bs.hand_e(card) && card != 8) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.trash[card - 1] += 1;
          next_bs.is_my_turn = !bs.is_my_turn;
          next_bs.reset_flag(false); //相手のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    }
  } else { // is_my_turn == true
    if(to_0p) {
      if(bs.barrier_s) {
        belief_state next_bs = bs;
        next_bs.is_wiz_choice = false;
        CW_BUMP(ef_wizard_elem);
        out.push(next_bs);
        return;
      }
      // open_e() はこのループの中で不変 (bs は const 参照) なので括り出す。
      const int open_card = bs.open_e();
      for(int card = 1; card <= 8; card++) {
        if(bs.deck(card, open_card)) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.is_my_turn = !bs.is_my_turn;
          next_bs.trash[bs.hand_s[0] - 1] += 1; //手札捨てる
          next_bs.hand_s[0] = card; //手札引く
          next_bs.reset_flag(true); //自分のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    } else {
      if(bs.open_e() == 8) {
        return;
      }
      // validate_hand_e_candidate(bs, "wiz");
      for(int card = 1; card <= 8; card++) {
        if(bs.hand_e(card) && card != 8) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.is_my_turn = !bs.is_my_turn;
          next_bs.trash[card - 1] += 1;
          next_bs.reset_flag(false); //相手のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    }
  }
}

void belief_state::print() const {
  // std::cout << "depth : " << depth << std::endl;
  std::cout << "barrier_s : " << barrier_s << " barrier_e : " << barrier_e << " is_my_turn : " << is_my_turn << std::endl;
  std::cout << "is_sol_choice : " << is_sol_choice << " is_wiz_choice : " << is_wiz_choice << " deck : " << count_deck() << std::endl;
  std::cout << "open_flag_e : " << open_flag_e << " sol_flag_e : " << sol_flag_e[0] << " " << sol_flag_e[1] << " lt5_flag_e : " << lt5_flag_e << " not7_flag_e : " << not7_flag_e << std::endl;
  std::cout << "open_flag_s : " << open_flag_s << " sol_flag_s : " << sol_flag_s[0] << " " << sol_flag_s[1] << " lt5_flag_s : " << lt5_flag_s << " not7_flag_s : " << not7_flag_s << std::endl;
  std::cout << "hand_s : " << hand_s[0] << " " << hand_s[1] << " ";
  std::cout << "trash:";
  for(int card = 1; card <= 8; card++) {
    std::cout << trash[card - 1] << " ";
  }
  std::cout << " deck_or_hand_e : ";
  for(int card = 1; card <= 8; card++) {
    std::cout << deck_or_hand_e(card) << " ";
  }
  std::cout << std::endl;
  std::cout << "hand_e: ";
  for(int card = 1; card <= 8; card++) {
    std::cout << hand_e(card) << " ";
  }
  std::cout << std::endl
            << std::endl;
}

[[noreturn]] void exit_with_print(const belief_state& bs, const char* context) {
  bs.print();
  std::cerr << "Error: " << context << std::endl;
  exit(2);
}

void validate_hand_e_candidate(const belief_state& bs, const char* context) {
  for(int card = 1; card <= 8; card++) {
    if(bs.hand_e(card)) return;
  }
  exit_with_print(bs, (std::string(context) + " : hand_e candidate is empty").c_str());
}

belief_state swap_player(const belief_state& bs, const int hand) {
  belief_state next_bs = bs;
  next_bs.is_my_turn = !bs.is_my_turn;
  std::swap(next_bs.barrier_s, next_bs.barrier_e);
  std::swap(next_bs.open_flag_e, next_bs.open_flag_s);
  std::swap(next_bs.sol_flag_e, next_bs.sol_flag_s);
  std::swap(next_bs.lt5_flag_e, next_bs.lt5_flag_s);
  std::swap(next_bs.not7_flag_e, next_bs.not7_flag_s);
  next_bs.hand_s[0] = hand;
  next_bs.hand_s[1] = 0;
  return next_bs;
}
#endif
