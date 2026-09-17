#ifndef BELIEF_STATE_LOSE_HPP
#define BELIEF_STATE_LOSE_HPP
#include "belief_state_win.hpp"

// use_lose / wizard_lose が draw_win を呼ぶため、win 側の checker への参照を持つ。
// 依存の向きは lose -> win の一方向 (ADR 0006)。
struct belief_state_lose_checker {
  explicit belief_state_lose_checker(belief_state_win_checker& w)
    : win(w) {}

  std::vector<std::pair<int, int>> is_lose(const belief_state& bs); // メモ化しない
  lose_result use_lose(const belief_state& bs, Card card);
  lose_result wizard_lose(const belief_state& bs, bool to_self);

private:
  lose_result use_lose_uncached(const belief_state& bs, Card card);
  lose_result wizard_lose_uncached(const belief_state& bs, bool to_self);

  belief_state_win_checker& win;
  std::unordered_map<unsigned long long, lose_result> m_use_lose, m_wizard_lose;
};

// <行動, 敗北するまでのターン数> の並びを返す。必敗の行動が無ければ空。
// 行動は、カード使用ノードなら使うカード (1〜8)、魔術師の対象選択ノードなら
// 0 = 自分 / 1 = 相手。ルール上すでに敗北している場合だけ 9 を1件返す。
// (この多目的のコードは未整理。行動ごとの真偽と手数を持つ形にする予定。)
std::vector<std::pair<int, int>> belief_state_lose_checker::is_lose(const belief_state& bs) {
  CW_BUMP(is_lose);

  if(bs.have_s(Card{7}) && bs.hand_s[1].has_value() && bs.hand_s[0].value().value() + bs.hand_s[1].value().value() >= 12) {
    return {{9, 0}};
  }
  // count_deck() は8要素ループなので、スカラの比較3つを先に評価する。
  // どれも副作用が無いので && の順序を入れ替えても意味は変わらない。
  if(!bs.hand_s[1].has_value() && !bs.is_wiz_choice && !bs.is_sol_choice && bs.count_deck() < 2) {
    MaybeCard min = bs.hand_e_min();
    if(min.value() > bs.hand_s[0].value()) return {{9, 0}};
    else return {};
  }
  std::vector<std::pair<int, int>> res;

  if(!bs.hand_s[1].has_value() && bs.is_sol_choice) return {};
  else if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
    auto res_self = wizard_lose(bs, true);
    auto res_enemy = wizard_lose(bs, false);

    if(res_self.is_lose) res.push_back({0, res_self.turns});
    if(res_enemy.is_lose) res.push_back({1, res_enemy.turns});
    return res;
  } else if(bs.is_my_turn && bs.hand_s[1].has_value()) {
    auto res0 = use_lose(bs, bs.hand_s[0].value());
    auto res1 = use_lose(bs, bs.hand_s[1].value());
    if(res0.is_lose) res.push_back({bs.hand_s[0].value().value(), res0.turns});
    if(res1.is_lose) res.push_back({bs.hand_s[1].value().value(), res1.turns});
    return res;
  } else return {};
}

lose_result belief_state_lose_checker::use_lose(const belief_state& bs, Card card) {
  const unsigned long long k = belief_state_win_checker::key(bs, card.value());
  if(!commentable_bs) {
    auto it = m_use_lose.find(k);
    if(it != m_use_lose.end()) return it->second;
  }
  auto r = use_lose_uncached(bs, card);
  if(!commentable_bs) m_use_lose.emplace(k, r);
  return r;
}

lose_result belief_state_lose_checker::use_lose_uncached(const belief_state& bs, Card card) {
  CW_BUMP(use_lose);
  // 姫(8) は捨てたら負け。自分の行動1つで負けが決まるので深さは 1。
  // use_win_uncached の if(card == Card{8}) return {false, 0}; と対になる。
  if(card == Card{8}) return {true, 1};
  if(card == Card{3} && !bs.barrier_e) {
    if(bs.hand_e_min().value() > bs.other_hand_s(Card{3}).value()) return {true, 1};
    if(bs.hand_e_min().value() < bs.other_hand_s(Card{3}).value()) return {false, 0};
  }
  // is_my_turn はここでは触らない (自分の手番のまま)。相手に手番が渡るのは
  // swap_player を通すときで、魔術師(5) は選択ノードを挟むので
  // wizard_lose の側で扱う。use_win_uncached と同じ形。
  struct belief_state next_bs = bs;
  next_bs.trash[card.index()] += 1; // 公開する

  // 手札を減らす
  if(bs.hand_s[0] == card) {
    next_bs.hand_s[0] = bs.hand_s[1];
    next_bs.hand_s[1] = MaybeCard();
  } else if(bs.hand_s[1] == card) {
    next_bs.hand_s[1] = MaybeCard();
  } else {
    std::cerr << "Error: card not in hand" << std::endl;
    exit(2);
  }

  next_bs = reset_flag_by_use(next_bs, true, card);

  if(card.value() >= 5) next_bs.not7_flag_s = true;

  // ANDノードの深さ集約用変数
  int max_f = -1;

  if(bs.barrier_e && card != Card{4} && card != Card{5} && card != Card{7}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(next_bs.hand_e(card2)) {
        struct belief_state next_bs2 = swap_player(next_bs, card2);
        auto res = win.draw_win(next_bs2);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == Card{1}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int c2 = 2; c2 <= 8; c2++) {
      Card card2{c2};
      if(bs.hand_e(card2)) return {false, 0};
    }
    if(bs.hand_e(Card{1})) {
      struct belief_state next_bs2 = swap_player(next_bs, Card{1});
      auto res = win.draw_win(next_bs2);
      //ランダム宣言のみ
      return {res.is_win, res.is_win ? res.turns + 1 : 0};
    }
  } else if(card == Card{2}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(bs.hand_e(card2)) {
        struct belief_state next_bs2 = swap_player(next_bs, card2);
        // next_bs2.open_flag_s = i + 1;//自分のフラグは不要
        auto res = win.draw_win(next_bs2);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == Card{3}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    Card other = bs.other_hand_s(Card{3}).value();
    if(bs.hand_e(other)) {
      struct belief_state next_bs2 = swap_player(next_bs, other);
      next_bs2.open_flag_e = other;
      // next_bs2.open_flag_s = other;//自分のフラグは不要
      auto res = win.draw_win(next_bs2);
      if(res.is_win) return {true, res.turns + 1};
    }
    return {false, 0};
  } else if(card == Card{4}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    next_bs.barrier_s = true;
    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(next_bs.hand_e(card2)) {
        struct belief_state next_bs2 = swap_player(next_bs, card2);
        auto res = win.draw_win(next_bs2);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == Card{5}) {
    auto res_self = wizard_lose(next_bs, true);
    auto res_enemy = wizard_lose(next_bs, false);

    bool has_true = res_self.is_lose && res_enemy.is_lose;

    int min_t = std::min(res_self.turns, res_enemy.turns);
    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == Card{6}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    Card other = bs.other_hand_s(Card{6}).value();
    next_bs.open_flag_e = other;
    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(bs.hand_e(card2)) {
        struct belief_state next_bs2 = next_bs;
        next_bs2.hand_s[0] = card2;
        next_bs2.open_flag_s = card2;
        struct belief_state next_bs3 = swap_player(next_bs2, other);
        auto res = win.draw_win(next_bs3);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == Card{7}) {
    // カードを出し終えたので相手の手番。この後 swap_player で視点も
    // 入れ替わるので、入れ替えたあとは「その視点の手番」= 真になる。
    next_bs.is_my_turn = false;
    next_bs.lt5_flag_s = true;
    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(next_bs.hand_e(card2)) {
        struct belief_state next_bs2 = swap_player(next_bs, card2);
        auto res = win.draw_win(next_bs2);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else return {false, 0};
      }
    }
    if(max_f == -1) return {false, 0};
    return {true, max_f + 1};
  }
  return {false, 0};
}

lose_result belief_state_lose_checker::wizard_lose(const belief_state& bs, bool to_self) {
  const unsigned long long k = belief_state_win_checker::key(bs, to_self ? 1 : 0);
  if(!commentable_bs) {
    auto it = m_wizard_lose.find(k);
    if(it != m_wizard_lose.end()) return it->second;
  }
  auto r = wizard_lose_uncached(bs, to_self);
  if(!commentable_bs) m_wizard_lose.emplace(k, r);
  return r;
}

lose_result belief_state_lose_checker::wizard_lose_uncached(const belief_state& bs, bool to_self) {
  CW_BUMP(wizard_lose);
  // 魔術師を使うのは自分。ef_wizard がこのフラグで枝を分ける。
  if(!bs.is_my_turn) exit_with_print(bs, "wizard_lose: is_my_turn が偽");
  // 自分を対象にすると手札を捨てて引き直す。それが姫(8) なら捨てた時点で負け。
  // ef_wizard はこの場合に候補を返さないので、preds.empty() の {false, 0} に
  // 落ちてしまう。必勝側は「候補が無い = 勝てない」で正しいが、必敗側は
  // 「候補が無い = 負け」なので、ここで先に答える。
  if(to_self && bs.hand_s[0] == Card{8}) return {true, 1};
  if(!to_self && bs.hand_e(Card{8}) && !bs.barrier_e) return {false, 0};
  // 深さは「自分の行動が何回続くか」。対象選択も1つの行動なので 0 ではなく 1。
  if(to_self && bs.hand_s[0] == Card{7}) return {true, 1};
  ef_wizard_preds preds;
  ef_wizard(bs, to_self, preds);
  if(preds.empty()) return {false, 0};
  int local_max_f = -1;
  for(const auto& p : preds) {
    validate_hand_e_candidate(p, "wizlose");
    for(int c = 1; c <= 8; c++) {
      Card card{c};
      if(p.hand_e(card)) {
        struct belief_state next_bs = swap_player(p, card);
        auto res = win.draw_win(next_bs);
        if(res.is_win) local_max_f = std::max(local_max_f, res.turns);
        else return {false, 0};
      }
    }
  }
  // 対象選択という自分の行動が1つ乗るので +1 する。
  return {true, local_max_f + 1};
}
#endif
