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
  MaybeCard open_flag_s; // 4bit
  MaybeCard open_flag_e; // 4bit
  MaybeCard sol_flag_s[2]; // 7bit
  MaybeCard sol_flag_e[2]; // 7bit
  MaybeCard hand_s[2]; // 6bit
  int trash[8]; // 14bit
  belief_state()
    : is_my_turn(false), is_sol_choice(false), is_wiz_choice(false), not7_flag_s(false), not7_flag_e(false),
      barrier_s(false), barrier_e(false), lt5_flag_s(false), lt5_flag_e(false), open_flag_s(), open_flag_e(),
      sol_flag_s{}, sol_flag_e{}, hand_s{}, trash{0, 0, 0, 0, 0, 0, 0, 0} {}
  belief_state(int open[3], std::string history, bool rnd);
  int deck_or_hand_e(Card card) const;
  bool hand_e(Card card) const;
  bool hand_s_est(Card card) const;
  MaybeCard open_e() const;
  MaybeCard open_s() const;
  bool deck(Card card) const;
  // open_e() は8要素ループの中で不変なので、括り出した値を渡せる版。
  bool deck(Card card, MaybeCard open_card) const;
  bool have_s(Card card) const;
  MaybeCard other_hand_s(Card card) const;
  int count_deck() const;
  MaybeCard hand_e_max() const;
  MaybeCard hand_e_min() const;
  MaybeCard deck_or_hand_e_min() const;
  void add_sol_s(Card card);
  void add_sol_e(Card card);
  void reset_flag(bool is_self);
  void print() const;
};

int trash_and_hand_s(const Card card, const MaybeCard hand[2], const int trash[8]);
int deck_or_hand_e(const Card card, const MaybeCard hand[2], const int trash[8]);
bool hand_e(const Card card, const MaybeCard hand[2], const int trash[8], const MaybeCard open_flag_e, const MaybeCard sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e);

belief_state reset_flag_by_use(const belief_state& bs, bool is_self, Card card);
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
belief_state draw(const belief_state& bs, Card draw_card);
belief_state swap_player(const belief_state& bs, Card hand);
[[noreturn]] void exit_with_print(const belief_state& bs, const char* context);
void validate_hand_e_candidate(const belief_state& bs, const char* context);

int trash_and_hand_s(const Card card, const MaybeCard hand[2], const int trash[8]) {
  return trash[card.index()] + (hand[0] == card ? 1 : 0) + (hand[1] == card ? 1 : 0);
}

int deck_or_hand_e(const Card card, const MaybeCard hand[2], const int trash[8]) {
  CW_BUMP(deck_or_hand_e);
  return max_num[card.index()] - trash_and_hand_s(card, hand, trash);
}

bool hand_e(const Card card, const MaybeCard hand[2], const int trash[8], const MaybeCard open_flag_e, const MaybeCard sol_flag_e[2], const bool lt5_flag_e, const bool not7_flag_e) {
  CW_BUMP(hand_e);
  if(open_flag_e.has_value()) { //手札が確定している場合
    if(open_flag_e.value() == card) {
      return true;
    } else {
      return false;
    }
  } else if(sol_flag_e[0].has_value() && sol_flag_e[0].value() == card) {
    return false;
  } else if(sol_flag_e[1].has_value() && sol_flag_e[1].value() == card) {
    return false;
  } else if(lt5_flag_e && card.value() >= 5) { //前のターンに7を出したときの,5以上
    return false;
  } else if(not7_flag_e && card == Card{7}) { //前のターンに5を出したときの,7
    return false;
  } else {
    return deck_or_hand_e(card, hand, trash) > 0;
  }
}

int belief_state::deck_or_hand_e(Card card) const {
  return ::deck_or_hand_e(card, hand_s, trash);
}

bool belief_state::hand_e(Card card) const {
  return ::hand_e(card, hand_s, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag_e);
}

bool belief_state::hand_s_est(Card card) const {
  MaybeCard enemy_hand[2] = {open_e(), MaybeCard()};
  return ::hand_e(card, enemy_hand, trash, open_flag_s, sol_flag_s, lt5_flag_s, not7_flag_s);
}

MaybeCard belief_state::open_e() const {
  CW_BUMP(open_e);
  MaybeCard found;
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(hand_e(card)) {
      if(!found.has_value()) {
        found = card;
      } else {
        return MaybeCard();
      }
    }
  }
  return found;
}

MaybeCard belief_state::open_s() const {
  MaybeCard found;
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(hand_s_est(card)) {
      if(!found.has_value()) {
        found = card;
      } else {
        return MaybeCard();
      }
    }
  }
  return found;
}

bool belief_state::deck(Card card, MaybeCard open_card) const {
  CW_BUMP(deck);
  return deck_or_hand_e(card) > (card == open_card ? 1 : 0);
}

bool belief_state::deck(Card card) const {
  return deck(card, open_e());
}

bool belief_state::have_s(Card card) const {
  return hand_s[0] == card || hand_s[1] == card;
}

MaybeCard belief_state::other_hand_s(Card card) const {
  if(hand_s[0] == card) {
    return hand_s[1];
  } else if(hand_s[1] == card) {
    return hand_s[0];
  } else {
    return MaybeCard();
  }
}

int belief_state::count_deck() const {
  CW_BUMP(count_deck);
  int count = 0;
  for(int c = 1; c <= 8; c++) {
    count += deck_or_hand_e(Card{c});
  }
  return count - 1;
}

MaybeCard belief_state::hand_e_max() const {
  CW_BUMP(hand_e_max);
  MaybeCard max_card;
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(hand_e(card)) {
      max_card = card;
    }
  }
  return max_card;
}

MaybeCard belief_state::hand_e_min() const {
  CW_BUMP(hand_e_min);
  MaybeCard min_card;
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(hand_e(card)) {
      if(!min_card.has_value()) {
        min_card = card;
      }
    }
  }
  return min_card;
}

MaybeCard belief_state::deck_or_hand_e_min() const {
  CW_BUMP(deck_or_hand_e_min);
  MaybeCard min_card;
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(deck_or_hand_e(card) > 0) {
      if(!min_card.has_value()) {
        min_card = card;
      }
    }
  }
  return min_card;
}

void belief_state::add_sol_s(Card card) {
  if(!sol_flag_s[0].has_value()) {
    sol_flag_s[0] = card;
  } else {
    if(sol_flag_s[0].value() < card) {
      sol_flag_s[1] = card;
    } else if(sol_flag_s[0].value() > card) {
      sol_flag_s[1] = sol_flag_s[0];
      sol_flag_s[0] = card;
    }
  }
}

void belief_state::add_sol_e(Card card) {
  if(!sol_flag_e[0].has_value()) {
    sol_flag_e[0] = card;
  } else {
    if(sol_flag_e[0].value() < card) {
      sol_flag_e[1] = card;
    } else if(sol_flag_e[0].value() > card) {
      sol_flag_e[1] = sol_flag_e[0];
      sol_flag_e[0] = card;
    }
  }
}

void belief_state::reset_flag(bool is_self) {
  if(is_self) {
    not7_flag_s = false;
    lt5_flag_s = false;
    open_flag_s = MaybeCard();
    sol_flag_s[0] = MaybeCard();
    sol_flag_s[1] = MaybeCard();
  } else {
    not7_flag_e = false;
    lt5_flag_e = false;
    open_flag_e = MaybeCard();
    sol_flag_e[0] = MaybeCard();
    sol_flag_e[1] = MaybeCard();
  }
}

belief_state reset_flag_by_use(const belief_state& bs, bool to_self, Card card) {
  CW_BUMP(reset_flag_by_use);
  struct belief_state next_bs = bs;
  if(to_self) {
    if(bs.open_flag_s.has_value() && bs.open_flag_s.value() == card) {
      next_bs.open_flag_s = MaybeCard();
    }
    if(bs.sol_flag_s[1].has_value() && bs.sol_flag_s[1].value() != card) {
      next_bs.sol_flag_s[1] = MaybeCard();
    }
    if(bs.sol_flag_s[0].has_value() && bs.sol_flag_s[0].value() != card) {
      next_bs.sol_flag_s[0] = next_bs.sol_flag_s[1];
      next_bs.sol_flag_s[1] = MaybeCard();
    }
    if(bs.lt5_flag_s && card.value() < 5) {
      next_bs.lt5_flag_s = false;
    }
    next_bs.not7_flag_s = false; //対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

  } else {
    if(bs.open_flag_e.has_value() && bs.open_flag_e.value() == card) {
      next_bs.open_flag_e = MaybeCard();
    }
    if(bs.sol_flag_e[1].has_value() && bs.sol_flag_e[1].value() != card) {
      next_bs.sol_flag_e[1] = MaybeCard();
    }
    if(bs.sol_flag_e[0].has_value() && bs.sol_flag_e[0].value() != card) {
      next_bs.sol_flag_e[0] = next_bs.sol_flag_e[1];
      next_bs.sol_flag_e[1] = MaybeCard();
    }
    if(bs.lt5_flag_e && card.value() < 5) {
      next_bs.lt5_flag_e = false;
    }
    next_bs.not7_flag_e = false;
  }
  return next_bs;
}

belief_state draw(const belief_state& bs, Card draw_card) {
  assert(bs.deck(draw_card) && bs.count_deck() > 0 && !bs.hand_s[1].has_value());
  belief_state next_bs = bs;
  next_bs.hand_s[1] = draw_card;
  return next_bs;
}

void ef_wizard(const belief_state& bs, bool to_0p, ef_wizard_preds& out) {
  CW_BUMP(ef_wizard);
  // is_my_turn は「いま手番を持っているのが視点プレイヤー (_s) か」。
  // 真なら自分が魔術師を使い、偽なら相手が使う。
  if(bs.is_my_turn) {
    if(to_0p) {
      if(bs.hand_s[0] == Card{8}) {
        // return false;
        return;
      }
      // open_e() はこのループの中で不変 (bs は const 参照) なので括り出す。
      const MaybeCard open_card = bs.open_e();
      for(int c = 1; c <= 8; c++) {
        Card card{c};
        if(bs.deck(card, open_card)) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.is_my_turn = false; // 自分が使ったので相手の手番
          next_bs.trash[bs.hand_s[0].value().index()] += 1; //手札捨てる
          next_bs.hand_s[0] = card; //手札引く
          next_bs.reset_flag(true); //自分のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    } else {
      if(bs.barrier_e) {
        // 僧侶(4)で守られているので何も起きない。何も起きなくても手番は渡る
        // ので、他の候補と同じく手番を渡す。
        belief_state next_bs = bs;
        next_bs.is_wiz_choice = false;
        next_bs.is_my_turn = false; // 自分が使ったので相手の手番
        CW_BUMP(ef_wizard_elem);
        out.push(next_bs);
        return;
      }
      // validate_hand_e_candidate(bs, "wiz");
      for(int c = 1; c <= 8; c++) {
        Card card{c};
        if(bs.hand_e(card) && card != Card{8}) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.trash[card.index()] += 1;
          next_bs.is_my_turn = false; // 自分が使ったので相手の手番
          next_bs.reset_flag(false); //相手のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    }
  } else { // is_my_turn == false: 相手が魔術師を使う
    if(to_0p) {
      if(bs.barrier_s) {
        // barrier_e 側と同じ理由で手番を渡す。
        belief_state next_bs = bs;
        next_bs.is_wiz_choice = false;
        next_bs.is_my_turn = true; // 相手が使ったので自分の手番
        CW_BUMP(ef_wizard_elem);
        out.push(next_bs);
        return;
      }
      // 守られておらず姫(8)を持っているなら、捨てさせられて自分の負け。
      // 魔術師で姫を捨てたプレイヤーが負けるルール。候補を返さないことで
      // 呼び出し側の AND ノードが偽になる。
      if(bs.hand_s[0] == Card{8}) return;
      // open_e() はこのループの中で不変 (bs は const 参照) なので括り出す。
      const MaybeCard open_card = bs.open_e();
      for(int c = 1; c <= 8; c++) {
        Card card{c};
        if(bs.deck(card, open_card)) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.is_my_turn = true; // 相手が使ったので自分の手番
          next_bs.trash[bs.hand_s[0].value().index()] += 1; //手札捨てる
          next_bs.hand_s[0] = card; //手札引く
          next_bs.reset_flag(true); //自分のフラグリセット
          CW_BUMP(ef_wizard_elem);
          out.push(next_bs);
        }
      }
      return;
    } else {
      if(bs.open_e() == Card{8}) {
        return;
      }
      // validate_hand_e_candidate(bs, "wiz");
      for(int c = 1; c <= 8; c++) {
        Card card{c};
        if(bs.hand_e(card) && card != Card{8}) {
          belief_state next_bs = bs;
          next_bs.is_wiz_choice = false;
          next_bs.is_my_turn = true; // 相手が使ったので自分の手番
          next_bs.trash[card.index()] += 1;
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
  std::cout << "open_flag_e : " << open_flag_e.raw() << " sol_flag_e : " << sol_flag_e[0].raw() << " " << sol_flag_e[1].raw() << " lt5_flag_e : " << lt5_flag_e << " not7_flag_e : " << not7_flag_e << std::endl;
  std::cout << "open_flag_s : " << open_flag_s.raw() << " sol_flag_s : " << sol_flag_s[0].raw() << " " << sol_flag_s[1].raw() << " lt5_flag_s : " << lt5_flag_s << " not7_flag_s : " << not7_flag_s << std::endl;
  std::cout << "hand_s : " << hand_s[0].raw() << " " << hand_s[1].raw() << " ";
  std::cout << "trash:";
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    std::cout << trash[card.index()] << " ";
  }
  std::cout << " deck_or_hand_e : ";
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    std::cout << deck_or_hand_e(card) << " ";
  }
  std::cout << std::endl;
  std::cout << "hand_e: ";
  for(int c = 1; c <= 8; c++) {
    Card card{c};
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
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(bs.hand_e(card)) return;
  }
  exit_with_print(bs, (std::string(context) + " : hand_e candidate is empty").c_str());
}

belief_state swap_player(const belief_state& bs, Card hand) {
  belief_state next_bs = bs;
  // 視点そのものを入れ替えるので、手番の持ち主も入れ替わる。
  // ここは「誰が行動したか」ではなく視点の反転なので定数にはできない。
  next_bs.is_my_turn = !bs.is_my_turn;
  std::swap(next_bs.barrier_s, next_bs.barrier_e);
  std::swap(next_bs.open_flag_e, next_bs.open_flag_s);
  std::swap(next_bs.sol_flag_e, next_bs.sol_flag_s);
  std::swap(next_bs.lt5_flag_e, next_bs.lt5_flag_s);
  std::swap(next_bs.not7_flag_e, next_bs.not7_flag_s);
  next_bs.hand_s[0] = hand;
  next_bs.hand_s[1] = MaybeCard();
  return next_bs;
}
#endif
