#ifndef BELIEF_STATE_LOSE_HPP
#define BELIEF_STATE_LOSE_HPP
#include "belief_state_win.hpp"

std::vector<std::pair<int, int>> is_lose(const belief_state& bs);
std::pair<bool, int> use_lose(const belief_state& bs, int card);
std::pair<bool, int> wiz_lose(const belief_state& bs, bool to0p);

//<使うカード, 敗北するまでのターン数>を返す。敗北しない場合は<0, 0>
//ルール上すでに敗北の場合9, 魔術師の使用による敗北で,対象自分のみなら15,対象相手のみなら25
std::vector<std::pair<int, int>> is_lose(const belief_state& bs) {
  CW_BUMP(is_lose);

  if(bs.have_s(7) && bs.hand_s[0] + bs.hand_s[1] >= 12) {
    return {{9, 0}};
  }
  // count_deck() は8要素ループなので、スカラの比較3つを先に評価する。
  // どれも副作用が無いので && の順序を入れ替えても意味は変わらない。
  if(bs.hand_s[1] == 0 && !bs.is_wiz_choice && !bs.is_sol_choice && bs.count_deck() < 2) {
    int min = bs.hand_e_min();
    if(min > bs.hand_s[0]) return {{9, 0}};
    else return {};
  }
  if(bs.have_s(8)) return {{8, 1}};

  std::vector<std::pair<int, int>> res;

  if(bs.hand_s[1] == 0 && bs.is_sol_choice) return {};
  else if(bs.hand_s[1] == 0 && bs.is_wiz_choice) {
    auto res_self = wiz_lose(bs, true);
    auto res_enemy = wiz_lose(bs, false);

    if(res_self.first) res.push_back({0, res_self.second});
    if(res_enemy.first) res.push_back({1, res_enemy.second});
    return res;
  } else if(bs.is_my_turn && bs.hand_s[1] != 0) {
    auto res0 = use_lose(bs, bs.hand_s[0]);
    auto res1 = use_lose(bs, bs.hand_s[1]);
    if(res0.first) res.push_back({bs.hand_s[0], res0.second});
    if(res1.first) res.push_back({bs.hand_s[1], res1.second});
    return res;
  } else return {};
}

std::pair<bool, int> use_lose(const belief_state& bs, int card) {
  CW_BUMP(use_lose);
  if(card == 3 && !bs.barrier_e) {
    if(bs.hand_e_min() > bs.other_hand_s(3)) return {true, 1};
    if(bs.hand_e_min() < bs.other_hand_s(3)) return {false, 0};
  }
  struct belief_state next_bs = bs;
  next_bs.is_my_turn = false;
  next_bs.trash[card - 1] += 1; // 公開する

  // 手札を減らす
  if(bs.hand_s[0] == card) {
    next_bs.hand_s[0] = bs.hand_s[1];
    next_bs.hand_s[1] = 0;
  } else if(bs.hand_s[1] == card) {
    next_bs.hand_s[1] = 0;
  } else {
    std::cerr << "Error: card not in hand" << std::endl;
    exit(2);
  }

  next_bs = reset_flag_by_use(next_bs, true, card);

  if(card >= 5) next_bs.not7_flag_s = true;

  // ANDノードの深さ集約用変数
  int max_f = -1;

  if(bs.barrier_e && card != 4 && card != 5 && card != 7) {
    for(int i = 0; i < 8; i++) {
      if(next_bs.hand_e(i)) {
        struct belief_state next_bs2 = swap_player(next_bs, i + 1);
        auto res = draw_win(next_bs2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 1) {
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int i = 1; i < 8; i++) {
      if(bs.hand_e(i)) return {false, 0};
    }
    if(bs.hand_e(0)) {
      struct belief_state next_bs2 = swap_player(next_bs, 1);
      auto res = draw_win(next_bs2);
      //ランダム宣言のみ
      return {res.first, res.first ? res.second + 1 : 0};
    }
  } else if(card == 2) {
    for(int i = 0; i < 8; i++) {
      if(bs.hand_e(i)) {
        struct belief_state next_bs2 = swap_player(next_bs, i + 1);
        // next_bs2.open_flag_s = i + 1;//自分のフラグは不要
        auto res = draw_win(next_bs2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 3) {
    int other = bs.other_hand_s(3);
    if(bs.hand_e(other - 1)) {
      struct belief_state next_bs2 = swap_player(next_bs, other);
      next_bs2.open_flag_e = other;
      // next_bs2.open_flag_s = other;//自分のフラグは不要
      auto res = draw_win(next_bs2);
      if(res.first) return {true, res.second + 1};
    }
    return {false, 0};
  } else if(card == 4) {
    next_bs.barrier_s = true;
    for(int i = 0; i < 8; i++) {
      if(next_bs.hand_e(i)) {
        struct belief_state next_bs2 = swap_player(next_bs, i + 1);
        auto res = draw_win(next_bs2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 5) {
    auto res_self = wiz_lose(next_bs, true);
    auto res_enemy = wiz_lose(next_bs, false);

    bool has_true = res_self.first && res_enemy.first;

    int min_t = std::min(res_self.second, res_enemy.second);
    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == 6) {
    int other = bs.other_hand_s(6);
    next_bs.open_flag_e = other;
    for(int i = 0; i < 8; i++) {
      if(bs.hand_e(i)) {
        struct belief_state next_bs2 = next_bs;
        next_bs2.hand_s[0] = i + 1;
        next_bs2.open_flag_s = i + 1;
        struct belief_state next_bs3 = swap_player(next_bs2, other);
        auto res = draw_win(next_bs3);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    return {true, max_f + 1};
  } else if(card == 7) {
    next_bs.lt5_flag_s = true;
    for(int i = 0; i < 8; i++) {
      if(next_bs.hand_e(i)) {
        struct belief_state next_bs2 = swap_player(next_bs, i + 1);
        auto res = draw_win(next_bs2);
        if(res.first) max_f = std::max(max_f, res.second);
        else return {false, 0};
      }
    }
    if(max_f == -1) return {false, 0};
    return {true, max_f + 1};
  }
  return {false, 0};
}

std::pair<bool, int> wiz_lose(const belief_state& bs, bool to0p) {
  CW_BUMP(wiz_lose);
  if(!to0p && bs.hand_e(7) && !bs.barrier_e) return {false, 0};
  if(to0p && bs.hand_s[0] == 7) return {true, 0};
  ef_wizard_preds preds;
  ef_wizard(bs, to0p, preds);
  if(preds.empty()) return {false, 0};
  int local_max_f = -1;
  for(const auto& p : preds) {
    validate_hand_e_candidate(p, "wizlose");
    for(int i = 0; i < 8; i++) {
      if(p.hand_e(i)) {
        struct belief_state next_bs = swap_player(p, i + 1);
        auto res = draw_win(next_bs);
        if(res.first) local_max_f = std::max(local_max_f, res.second);
        else return {false, 0};
      }
    }
  }
  return {true, local_max_f};
}
#endif
