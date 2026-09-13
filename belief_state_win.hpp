#ifndef BELIEF_STATE_WIN_HPP
#define BELIEF_STATE_WIN_HPP
#include "belief_state.hpp"
#include <unordered_map>

std::pair<int, int> is_terminated_win(const belief_state& bs);
std::vector<int> able_actions(const belief_state& bs, int card, bool is_second_player);
int action_count(const belief_state& bs);

// belief_state と引数以外の可変状態を読まない純関数なので、5本はメモ化する。
// メモ表は判定ごとに分ける (関数が違えば同じ (bs, extra) でも答えが違うため)。
struct belief_state_win_checker {
  // belief_state の16フィールドと追加の鍵 (card 0..8 / to0p) を 59bit に詰める。
  // 値域: open_flag / sol_flag / hand_s は 0..8、trash[i] は max_num[i] 以下。
  static unsigned long long key(const belief_state& bs, int extra);

  std::pair<int, int> is_win(const belief_state& bs); // メモ化しない (入口、再帰しない)
  std::pair<bool, int> use_win(const belief_state& bs, Card card);
  std::pair<bool, int> enemy_turn_win(const belief_state& bs);
  std::pair<bool, int> draw_win(const belief_state& bs);
  std::pair<bool, int> sol_win(const belief_state& bs, Card card);
  std::pair<bool, int> wiz_win(const belief_state& bs, bool to0p);

private:
  std::pair<bool, int> use_win_impl(const belief_state& bs, Card card);
  std::pair<bool, int> enemy_turn_win_impl(const belief_state& bs);
  std::pair<bool, int> draw_win_impl(const belief_state& bs);
  std::pair<bool, int> sol_win_impl(const belief_state& bs, Card card);
  std::pair<bool, int> wiz_win_impl(const belief_state& bs, bool to0p);

  std::unordered_map<unsigned long long, std::pair<bool, int>> m_use_win, m_enemy_turn_win,
      m_draw_win, m_sol_win, m_wiz_win;
};

unsigned long long belief_state_win_checker::key(const belief_state& bs, int extra) {
  unsigned long long k = 0;
  int b = 0;
  auto put = [&](unsigned long long v, int w) { k |= v << b; b += w; };
  put(bs.is_my_turn, 1);
  put(bs.is_sol_choice, 1);
  put(bs.is_wiz_choice, 1);
  put(bs.not7_flag_s, 1);
  put(bs.not7_flag_e, 1);
  put(bs.barrier_s, 1);
  put(bs.barrier_e, 1);
  put(bs.lt5_flag_s, 1);
  put(bs.lt5_flag_e, 1);
  put(bs.open_flag_s.raw(), 4);
  put(bs.open_flag_e.raw(), 4);
  put(bs.sol_flag_s[0].raw(), 4);
  put(bs.sol_flag_s[1].raw(), 4);
  put(bs.sol_flag_e[0].raw(), 4);
  put(bs.sol_flag_e[1].raw(), 4);
  put(bs.hand_s[0].raw(), 4);
  put(bs.hand_s[1].raw(), 4);
  static const int tw[8] = {3, 2, 2, 2, 2, 1, 1, 1}; // max_num = {5,2,2,2,2,1,1,1}
  for(int i = 0; i < 8; i++) put(bs.trash[i], tw[i]);
  put(extra, 4);
  return k;
}

std::pair<int, int> is_terminated_win(const belief_state& bs) {
  CW_BUMP(is_terminated_win);
  if(bs.have_s(Card{7}) && bs.hand_s[1].has_value() && bs.hand_s[0].value().value() + bs.hand_s[1].value().value() >= 12) {
    return {0, 0};
  }
  // count_deck() は8要素ループなので、スカラの比較3つを先に評価する。
  // どれも副作用が無いので && の順序を入れ替えても意味は変わらない。
  if(!bs.hand_s[1].has_value() && !bs.is_wiz_choice && !bs.is_sol_choice && bs.count_deck() < 2) {
    MaybeCard max = bs.hand_e_max();
    // if(max == 0) {std::cout << "Error: max card is 0" << std::endl; bs.print(); exit(1);}
    if(max.value() < bs.hand_s[0].value()) return {bs.hand_s[0].value().value(), 0};
    else return {0, 0};
  }
  if(!bs.barrier_e && bs.hand_s[1].has_value()) {
    if(bs.have_s(Card{1})) {
      MaybeCard oe = bs.open_e();
      if(oe.has_value() && oe.value() != Card{1}) return {1, 1};
    }
    if(bs.have_s(Card{3}) && bs.hand_e_max().value() < bs.other_hand_s(Card{3}).value()) {
      return {3, 1};
    }
    if(bs.have_s(Card{5}) && bs.open_e() == Card{8}) {
      return {5, 1};
    }
  }
  return {-1, 0};
}

std::pair<int, int> belief_state_win_checker::is_win(const belief_state& bs) {
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  std::pair<int, int> res;
  if(!bs.hand_s[1].has_value() && bs.is_sol_choice) {
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int c = 2; c <= 8; c++) {
      Card card{c};
      if(bs.hand_e(card)) {
        auto res_sol = sol_win(bs, card);
        if(res_sol.first) {
          has_true = true;
          min_t = std::min(min_t, res_sol.second);
        }
      }
    }
    return {has_true, has_true ? min_t + 1 : 0};

  } else if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
    auto res_self = wiz_win(bs, true);
    auto res_enemy = wiz_win(bs, false);

    bool has_true = res_self.first || res_enemy.first;

    int min_t = 1e9;
    if(res_self.first) min_t = std::min(min_t, res_self.second);
    if(res_enemy.first) min_t = std::min(min_t, res_enemy.second);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(bs.is_my_turn && bs.hand_s[1].has_value()) {
    // 手札が2枚あり、両方同じカードの場合（片方だけ評価して無駄を省く）
    if(bs.hand_s[0] == bs.hand_s[1]) {
      res = use_win(bs, bs.hand_s[0].value());
    }
    // 手札が2枚あり、違うカードの場合（ORノードの評価）
    else {
      auto res0 = use_win(bs, bs.hand_s[0].value());
      auto res1 = use_win(bs, bs.hand_s[1].value());
      if(res0.first && res1.first) {
        if(res0.second < res1.second) {
          res.first = bs.hand_s[0].value().value();
          res.second = res0.second;
        } else {
          res.first = bs.hand_s[1].value().value();
          res.second = res1.second;
        }
      } else if(res0.first) {
        res.first = bs.hand_s[0].value().value();
        res.second = res0.second;
      } else if(res1.first) {
        res.first = bs.hand_s[1].value().value();
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

std::pair<bool, int> belief_state_win_checker::use_win(const belief_state& bs, Card card) {
  const unsigned long long k = key(bs, card.value());
  if(!commentable_bs) {
    auto it = m_use_win.find(k);
    if(it != m_use_win.end()) return it->second;
  }
  auto r = use_win_impl(bs, card);
  if(!commentable_bs) m_use_win.emplace(k, r);
  return r;
}

std::pair<bool, int> belief_state_win_checker::use_win_impl(const belief_state& bs, Card card) {
  CW_BUMP(use_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  if(card == Card{3} && !bs.barrier_s && bs.hand_e_min().value() > bs.other_hand_s(Card{3}).value()) {
    return {false, 0};
  }
  if(card == Card{8}) return {false, 0};
  if(commentable_bs) std::cout << bs.count_deck() << "use :" << card.value() << std::endl;

  struct belief_state next_bs = bs;
  next_bs.is_my_turn = !bs.is_my_turn;
  next_bs.trash[card.index()] += 1; //公開する
  //手札を減らす
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

  // if(card == 5 || (card == 6 && bs.barrier_e)) next_bs.not7_flag_s = true;//自分のフラグは不要

  if(bs.barrier_e && card != Card{4} && card != Card{5} && card != Card{7}) {
    auto res = enemy_turn_win(next_bs);
    return {res.first, res.first ? res.second + 1 : 0};
  } else if(card == Card{1}) {
    next_bs.is_sol_choice = true;
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int c2 = 2; c2 <= 8; c2++) {
      Card card2{c2};
      if(bs.hand_e(card2)) {
        auto res = sol_win(next_bs, card2);
        if(res.first) {
          has_true = true;
          min_t = std::min(min_t, res.second);
        }
      }
    }
    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == Card{2}) {
    bool all_true = true;
    int max_f = -1;

    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(all_true)
        if(bs.hand_e(card2)) {
          struct belief_state next_bs2 = next_bs;
          next_bs2.open_flag_e = card2;
          auto res = enemy_turn_win(next_bs2);
          if(res.first) {
            max_f = std::max(max_f, res.second);
          } else {
            all_true = false;
          }
        }
    }
    if(max_f == -1) return {false, 0};
    return {all_true, all_true ? max_f + 1 : 0};
  } else if(card == Card{3}) {
    Card other = bs.other_hand_s(Card{3}).value();
    if(bs.hand_e(other)) {
      struct belief_state next_bs2 = next_bs;
      next_bs2.open_flag_e = other;
      // next_bs2.open_flag_s = other;//自分のフラグは不要
      auto res = enemy_turn_win(next_bs2);
      return {res.first, res.first ? res.second + 1 : 0};
    }
    return {false, 0};
  } else if(card == Card{4}) {
    next_bs.barrier_s = true;
    auto res = enemy_turn_win(next_bs);
    return {res.first, res.first ? res.second + 1 : 0};
  } else if(card == Card{5}) {
    next_bs.is_wiz_choice = true;
    auto res_self = wiz_win(next_bs, true);
    auto res_enemy = wiz_win(next_bs, false);

    bool has_true = res_self.first || res_enemy.first;

    int min_t = 1e9;
    if(res_self.first) min_t = std::min(min_t, res_self.second);
    if(res_enemy.first) min_t = std::min(min_t, res_enemy.second);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == Card{6}) {
    next_bs.reset_flag(true);
    next_bs.reset_flag(false);
    next_bs.open_flag_e = bs.other_hand_s(card).value();
    bool all_true = true;
    int max_f = -1;

    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(bs.hand_e(card2)) {
        struct belief_state next_bs2 = next_bs;
        next_bs2.hand_s[0] = card2;
        // next_bs2.open_flag_s = i + 1;//自分のフラグは不要
        auto res = enemy_turn_win(next_bs2);
        if(res.first) {
          max_f = std::max(max_f, res.second);
        } else {
          all_true = false;
        }
      }
    }
    if(max_f == -1) return {false, 0};
    return {all_true, all_true ? max_f + 1 : 0};
  } else if(card == Card{7}) {
    // next_bs.lt5_flag_s = true;//自分のフラグは不要
    auto res = enemy_turn_win(next_bs);
    return {res.first, res.first ? res.second + 1 : 0};
  } else return {false, 0};
}

std::pair<bool, int> belief_state_win_checker::enemy_turn_win(const belief_state& bs) {
  const unsigned long long k = key(bs, 0);
  if(!commentable_bs) {
    auto it = m_enemy_turn_win.find(k);
    if(it != m_enemy_turn_win.end()) return it->second;
  }
  auto r = enemy_turn_win_impl(bs);
  if(!commentable_bs) m_enemy_turn_win.emplace(k, r);
  return r;
}

std::pair<bool, int> belief_state_win_checker::enemy_turn_win_impl(const belief_state& bs) {
  CW_BUMP(enemy_turn_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  bool all_true = true;
  int max_f = -1;

  for(int c = 1; c <= 7; c++) {
    Card card{c};
    if(!all_true) break;
    if(bs.deck_or_hand_e(card) > 0) {
      if(commentable_bs) std::cout << bs.count_deck() - 1 << "enemy :" << card.value() << std::endl;
      // 1. カード効果による即時敗北（深さ0の敗北として扱う）
      if(card == Card{1} && !bs.barrier_s && bs.hand_s[0].value() != Card{1}) {
        all_true = false;
        continue;
      }

      if(card == Card{3} && !bs.barrier_s) {
        bool immediate_loss = false;
        for(int c2 = 1; c2 <= 8; c2++) {
          Card card2{c2};
          if(bs.deck_or_hand_e(card2)) {
            if(card2 == Card{3} && bs.deck_or_hand_e(Card{3}) >= 2 && bs.hand_s[0].value() < Card{3}) {
              immediate_loss = true;
              break;
            } else if(card2.value() > bs.hand_s[0].value().value()) {
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

      if(card == Card{5} && bs.hand_s[0] == Card{8} && !bs.barrier_s) {
        all_true = false;
        continue;
      }

      // 2. 状態の更新
      struct belief_state next_bs = bs;
      next_bs.trash[card.index()] += 1;
      next_bs.barrier_e = false;
      next_bs.is_my_turn = !bs.is_my_turn;
      next_bs = reset_flag_by_use(next_bs, false, card);

      if(card == Card{5}) next_bs.not7_flag_e = true;

      // 3. 各カードごとの再帰評価
      if(card == Card{3}) {
        if(!bs.barrier_s) next_bs.open_flag_e = bs.hand_s[0].value();
        auto res = draw_win(next_bs);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else if(card == Card{4}) {
        next_bs.barrier_e = true;
        auto res = draw_win(next_bs);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else if(card == Card{5}) {
        if(bs.open_e() == Card{7}) continue;

        // 魔術師専用の集約ラムダ
        auto eval_wiz_preds = [&](const ef_wizard_preds& preds) -> std::pair<bool, int> {
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

        ef_wizard_preds preds_toself;
        ef_wizard(next_bs, true, preds_toself);
        auto res_self = eval_wiz_preds(preds_toself);

        ef_wizard_preds preds_toenemy;
        ef_wizard(next_bs, false, preds_toenemy);
        auto res_enemy = eval_wiz_preds(preds_toenemy);

        if(res_self.first && res_enemy.first) max_f = std::max(res_self.second, res_enemy.second);
        else all_true = false;
      } else if(card == Card{6}) {
        if(bs.open_e() == Card{7}) continue;
        if(!bs.barrier_s) {
          bool gene_all_true = true;
          int gene_max_f = -1;

          for(int c2 = 1; c2 <= 8; c2++) {
            Card card2{c2};
            if(!gene_all_true) break;
            if(next_bs.hand_e(card2)) {
              struct belief_state next_bs2 = next_bs;
              next_bs2.hand_s[0] = card2;
              next_bs2.reset_flag(true);
              next_bs2.reset_flag(false);
              next_bs2.open_flag_e = bs.hand_s[0].value();
              auto res = draw_win(next_bs2);
              if(res.first) gene_max_f = std::max(gene_max_f, res.second);
              else gene_all_true = false;
            }
          }
          if(gene_all_true && gene_max_f != -1) max_f = std::max(max_f, gene_max_f);
          else all_true = false;
        } else {
          next_bs.not7_flag_e = true;
          auto res = draw_win(next_bs);
          if(res.first) max_f = std::max(max_f, res.second);
          else all_true = false;
        }
      } else if(card == Card{7}) {
        next_bs.lt5_flag_e = true;

        // 7 を使った相手は、使用後も 5 未満のカードを手札に残している必要がある。
        // deck_or_hand_e() に 7 自身が存在するだけでは、この枝は成立しない。
        bool has_remaining_hand_candidate = false;
        for(int c2 = 1; c2 <= 8; c2++) {
          Card card2{c2};
          if(next_bs.hand_e(card2)) {
            has_remaining_hand_candidate = true;
            break;
          }
        }
        if(!has_remaining_hand_candidate) continue;

        auto res = draw_win(next_bs);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else {
        auto res = draw_win(next_bs);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      }
    }
  }

  // 指定された win に応じて最適な深さを +1 して返す
  return {all_true, all_true ? max_f + 1 : 0};
}

std::pair<bool, int> belief_state_win_checker::draw_win(const belief_state& bs) {
  const unsigned long long k = key(bs, 0);
  if(!commentable_bs) {
    auto it = m_draw_win.find(k);
    if(it != m_draw_win.end()) return it->second;
  }
  auto r = draw_win_impl(bs);
  if(!commentable_bs) m_draw_win.emplace(k, r);
  return r;
}

std::pair<bool, int> belief_state_win_checker::draw_win_impl(const belief_state& bs) {
  CW_BUMP(draw_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  bool all_true = true;
  int max_f = -1;

  // open_e() はこのループの中で不変 (bs は const 参照) なので括り出す。
  const MaybeCard open_card = bs.open_e();
  for(int c = 1; c <= 8; c++) {
    Card card{c};
    if(!all_true) break;
    if(bs.deck(card, open_card)) {
      belief_state next_bs = draw(bs, card);
      next_bs.barrier_s = false;
      next_bs.is_my_turn = !next_bs.is_my_turn;
      if(commentable_bs) std::cout << next_bs.count_deck() << "draw " << card.value() << std::endl;

      // --- 自分の手札の選択 (ORノード) ---
      auto res0 = use_win(next_bs, next_bs.hand_s[0].value());

      bool or_first = false;
      int or_second = 0;

      // 手札の2枚が違うカードなら、もう一方も評価する
      if(next_bs.hand_s[0] != card) {
        auto res1 = use_win(next_bs, next_bs.hand_s[1].value());

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

std::pair<bool, int> belief_state_win_checker::sol_win(const belief_state& bs, Card card) {
  const unsigned long long k = key(bs, card.value());
  if(!commentable_bs) {
    auto it = m_sol_win.find(k);
    if(it != m_sol_win.end()) return it->second;
  }
  auto r = sol_win_impl(bs, card);
  if(!commentable_bs) m_sol_win.emplace(k, r);
  return r;
}

std::pair<bool, int> belief_state_win_checker::sol_win_impl(const belief_state& bs, Card card) {
  CW_BUMP(sol_win);
  if(!bs.is_sol_choice) exit_with_print(bs, "sol_win called when not in sol_choice state");
  if(bs.open_e() == card && !bs.barrier_e) return {true, 1};
  struct belief_state next_bs = bs;
  // 宣言が確定したので、保留中の兵士選択を解除する。
  next_bs.is_sol_choice = false;
  next_bs.add_sol_e(card);
  auto res = enemy_turn_win(next_bs);
  return {res.first, res.first ? res.second + 1 : 0};
}

std::pair<bool, int> belief_state_win_checker::wiz_win(const belief_state& bs, bool to0p) {
  const unsigned long long k = key(bs, to0p ? 1 : 0);
  if(!commentable_bs) {
    auto it = m_wiz_win.find(k);
    if(it != m_wiz_win.end()) return it->second;
  }
  auto r = wiz_win_impl(bs, to0p);
  if(!commentable_bs) m_wiz_win.emplace(k, r);
  return r;
}

std::pair<bool, int> belief_state_win_checker::wiz_win_impl(const belief_state& bs, bool to0p) {
  CW_BUMP(wiz_win);
  if(!bs.is_wiz_choice) exit_with_print(bs, "wiz_win called when not in wiz_choice state");
  if(!to0p && bs.open_e() == Card{8} && !bs.barrier_e) return {true, 1};
  if(to0p && bs.hand_s[0] == Card{7}) return {false, 0};
  struct belief_state next_bs = bs;
  next_bs.is_wiz_choice = false;
  ef_wizard_preds preds;
  ef_wizard(next_bs, to0p, preds);
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

// 行動コードは 0-origin のカード添字をそのまま桁に埋め込むシリアライズ形式
// なので、この関数の中だけは添字 (0..7) で通す。Belief State のアクセサは
// カード (1..8) を取るため、呼び出しでだけ +1 する。
std::vector<int> able_actions(const belief_state& bs, int card, bool is_second_player) {
  int base = 40 + card - 1;
  std::vector<int> actions;

  if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
    // --- 自分を対象とする場合 ---
    if((card == 0 && !is_second_player) || (card == 1 && is_second_player)) {
      // ここの card は実カードではなく 0/1 の対象選択フラグなので、Card を作れない
      // (値域 1..8 を破る)。元のコードは other_hand_s(card) を呼んでおり、この分岐は
      // hand_s[1] が無いことが前提なので、card==0 なら hand_s[0]、card==1 なら
      // どちらの枝にも該当せず「無し」(生の値 0) を返す。その結果を直接書く。
      const int other_raw = (card == 0) ? bs.hand_s[0].raw() : 0;
      for(int j = 0; j < 8; j++)
        if(bs.deck(Card{j + 1})) actions.push_back(44000 + is_second_player * 100 + (other_raw - 1) * 10 + j);
    }
    // --- 相手を対象とする場合 ---
    else if((card == 0 && is_second_player) || (card == 1 && !is_second_player)) {
      if(!bs.barrier_e) {
        for(int i = 0; i < 7; i++) {
          if(bs.hand_e(Card{i + 1})) { // 相手が捨てさせられるカード
            actions.push_back(44000 + !is_second_player * 100 + i * 10 + 0);
          }
        }
      } else actions.push_back(44000 + !is_second_player * 100);
    }
  } else if(card == 4 || card == 7 || bs.barrier_e || card == 1) {
    actions.push_back(base);
  } else if(card == 2) {
    // 相手の判明するカード
    for(int i = 0; i < 8; i++)
      if(bs.hand_e(Card{i + 1})) actions.push_back(base * 10 + i);
  } else if(card == 3) {
    // 相手の判明するカード
    int other_i = bs.other_hand_s(Card{card}).value().index();
    if(bs.hand_e(Card{other_i + 1})) actions.push_back(base * 100 + other_i * 11);
  } else if(card == 6) {
    // 相手と交換するカード
    for(int i = 0; i < 8; i++)
      if(bs.hand_e(Card{i + 1})) actions.push_back(base * 100 + (bs.other_hand_s(Card{card}).value().index()) * 10 + i);
  }

  return actions;
}

int action_count(const belief_state& bs) {
  if(bs.is_sol_choice) {
    int count = 0;
    for(int c = 2; c <= 8; c++) {
      if(bs.deck_or_hand_e(Card{c})) count++;
    }
    return count;
  }
  if(bs.is_wiz_choice) return 2;

  if(!bs.hand_s[1].has_value()) return 0;
  return bs.hand_s[0] == bs.hand_s[1] ? 1 : 2;
}
#endif
