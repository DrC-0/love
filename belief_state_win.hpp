#ifndef BELIEF_STATE_WIN_HPP
#define BELIEF_STATE_WIN_HPP
#include "belief_state.hpp"

std::pair<int, int> is_win(const belief_state& bs);
std::pair<int, int> is_terminated_win(const belief_state& bs);
std::pair<bool, int> use_win(const belief_state& bs, int card);
std::pair<bool, int> draw_win(const belief_state& bs);
std::pair<bool, int> enemy_turn_win(const belief_state& bs);
std::pair<bool, int> sol_win(const belief_state& bs, int card);
std::pair<bool, int> wiz_win(const belief_state& bs, bool to0p);
std::vector<int> able_actions(const belief_state& bs, int card, bool is_second_player);
int action_count(const belief_state& bs);

std::pair<int, int> is_terminated_win(const belief_state& bs) {
  CW_BUMP(is_terminated_win);
  if(bs.have_s(7) && bs.hand_s[0] + bs.hand_s[1] >= 12) {
    return {0, 0};
  }
  // count_deck() は8要素ループなので、スカラの比較3つを先に評価する。
  // どれも副作用が無いので && の順序を入れ替えても意味は変わらない。
  if(bs.hand_s[1] == 0 && !bs.is_wiz_choice && !bs.is_sol_choice && bs.count_deck() < 2) {
    int max = bs.hand_e_max();
    // if(max == 0) {std::cout << "Error: max card is 0" << std::endl; bs.print(); exit(1);}
    if(max < bs.hand_s[0]) return {bs.hand_s[0], 0};
    else return {0, 0};
  }
  if(!bs.barrier_e && bs.hand_s[1] > 0) {
    if(bs.have_s(1) && bs.open_e() > 1) {
      return {1, 1};
    }
    if(bs.have_s(3) && bs.hand_e_max() < bs.other_hand_s(3)) {
      return {3, 1};
    }
    if(bs.have_s(5) && bs.open_e() == 8) {
      return {5, 1};
    }
  }
  return {-1, 0};
}

std::pair<int, int> is_win(const belief_state& bs) {
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  std::pair<int, int> res;
  if(bs.hand_s[1] == 0 && bs.is_sol_choice) {
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int i = 1; i < 8; i++) {
      if(bs.hand_e(i)) {
        auto res_sol = sol_win(bs, i + 1);
        if(res_sol.first) {
          has_true = true;
          min_t = std::min(min_t, res_sol.second);
        }
      }
    }
    return {has_true, has_true ? min_t + 1 : 0};

  } else if(bs.hand_s[1] == 0 && bs.is_wiz_choice) {
    auto res_self = wiz_win(bs, true);
    auto res_enemy = wiz_win(bs, false);

    bool has_true = res_self.first || res_enemy.first;

    int min_t = 1e9;
    if(res_self.first) min_t = std::min(min_t, res_self.second);
    if(res_enemy.first) min_t = std::min(min_t, res_enemy.second);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(bs.is_my_turn && bs.hand_s[1] != 0) {
    // 手札が2枚あり、両方同じカードの場合（片方だけ評価して無駄を省く）
    if(bs.hand_s[0] == bs.hand_s[1]) {
      res = use_win(bs, bs.hand_s[0]);
    }
    // 手札が2枚あり、違うカードの場合（ORノードの評価）
    else {
      auto res0 = use_win(bs, bs.hand_s[0]);
      auto res1 = use_win(bs, bs.hand_s[1]);
      if(res0.first && res1.first) {
        if(res0.second < res1.second) {
          res.first = bs.hand_s[0];
          res.second = res0.second;
        } else {
          res.first = bs.hand_s[1];
          res.second = res1.second;
        }
      } else if(res0.first) {
        res.first = bs.hand_s[0];
        res.second = res0.second;
      } else if(res1.first) {
        res.first = bs.hand_s[1];
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

std::pair<bool, int> use_win(const belief_state& bs, int card) {
  CW_BUMP(use_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  if(card == 3 && !bs.barrier_s && bs.hand_e_min() > bs.other_hand_s(3)) {
    return {false, 0};
  }
  if(card == 8) return {false, 0};
  if(commentable_bs) std::cout << bs.count_deck() << "use :" << card << std::endl;

  struct belief_state next_bs = bs;
  next_bs.is_my_turn = !bs.is_my_turn;
  next_bs.trash[card - 1] += 1; //公開する
  //手札を減らす
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

  // if(card == 5 || (card == 6 && bs.barrier_e)) next_bs.not7_flag_s = true;//自分のフラグは不要

  if(bs.barrier_e && card != 4 && card != 5 && card != 7) {
    auto res = enemy_turn_win(next_bs);
    return {res.first, res.first ? res.second + 1 : 0};
  } else if(card == 1) {
    next_bs.is_sol_choice = true;
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int i = 1; i < 8; i++) {
      if(bs.hand_e(i)) {
        auto res = sol_win(next_bs, i + 1);
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
        if(bs.hand_e(i)) {
          struct belief_state next_bs2 = next_bs;
          next_bs2.open_flag_e = i + 1;
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
  } else if(card == 3) {
    int other = bs.other_hand_s(3);
    if(bs.hand_e(other - 1)) {
      struct belief_state next_bs2 = next_bs;
      next_bs2.open_flag_e = other;
      // next_bs2.open_flag_s = other;//自分のフラグは不要
      auto res = enemy_turn_win(next_bs2);
      return {res.first, res.first ? res.second + 1 : 0};
    }
    return {false, 0};
  } else if(card == 4) {
    next_bs.barrier_s = true;
    auto res = enemy_turn_win(next_bs);
    return {res.first, res.first ? res.second + 1 : 0};
  } else if(card == 5) {
    next_bs.is_wiz_choice = true;
    auto res_self = wiz_win(next_bs, true);
    auto res_enemy = wiz_win(next_bs, false);

    bool has_true = res_self.first || res_enemy.first;

    int min_t = 1e9;
    if(res_self.first) min_t = std::min(min_t, res_self.second);
    if(res_enemy.first) min_t = std::min(min_t, res_enemy.second);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == 6) {
    next_bs.reset_flag(true);
    next_bs.reset_flag(false);
    next_bs.open_flag_e = bs.other_hand_s(card);
    bool all_true = true;
    int max_f = -1;

    for(int i = 0; i < 8; i++) {
      if(bs.hand_e(i)) {
        struct belief_state next_bs2 = next_bs;
        next_bs2.hand_s[0] = i + 1;
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
  } else if(card == 7) {
    // next_bs.lt5_flag_s = true;//自分のフラグは不要
    auto res = enemy_turn_win(next_bs);
    return {res.first, res.first ? res.second + 1 : 0};
  } else return {false, 0};
}

std::pair<bool, int> enemy_turn_win(const belief_state& bs) {
  CW_BUMP(enemy_turn_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  bool all_true = true;
  int max_f = -1;

  for(int i = 0; i < 7; i++) {
    if(!all_true) break;
    if(bs.deck_or_hand_e(i) > 0) {
      if(commentable_bs) std::cout << bs.count_deck() - 1 << "enemy :" << i + 1 << std::endl;
      // 1. カード効果による即時敗北（深さ0の敗北として扱う）
      if(i + 1 == 1 && !bs.barrier_s && bs.hand_s[0] > 1) {
        all_true = false;
        continue;
      }

      if(i + 1 == 3 && !bs.barrier_s) {
        bool immediate_loss = false;
        for(int j = 0; j < 8; j++) {
          if(bs.deck_or_hand_e(j)) {
            if(j + 1 == 3 && bs.deck_or_hand_e(2) >= 2 && bs.hand_s[0] < 3) {
              immediate_loss = true;
              break;
            } else if(j + 1 > bs.hand_s[0]) {
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

      if(i + 1 == 5 && bs.hand_s[0] == 8 && !bs.barrier_s) {
        all_true = false;
        continue;
      }

      // 2. 状態の更新
      struct belief_state next_bs = bs;
      next_bs.trash[i] += 1;
      next_bs.barrier_e = false;
      next_bs.is_my_turn = !bs.is_my_turn;
      next_bs = reset_flag_by_use(next_bs, false, i + 1);

      if(i + 1 == 5) next_bs.not7_flag_e = true;

      // 3. 各カードごとの再帰評価
      if(i + 1 == 3) {
        if(!bs.barrier_s) next_bs.open_flag_e = bs.hand_s[0];
        auto res = draw_win(next_bs);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else if(i + 1 == 4) {
        next_bs.barrier_e = true;
        auto res = draw_win(next_bs);
        if(res.first) max_f = std::max(max_f, res.second);
        else all_true = false;
      } else if(i + 1 == 5) {
        if(bs.open_e() == 7) continue;

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
      } else if(i + 1 == 6) {
        if(bs.open_e() == 7) continue;
        if(!bs.barrier_s) {
          bool gene_all_true = true;
          int gene_max_f = -1;

          for(int j = 0; j < 8; j++) {
            if(!gene_all_true) break;
            if(next_bs.hand_e(j)) {
              struct belief_state next_bs2 = next_bs;
              next_bs2.hand_s[0] = j + 1;
              next_bs2.reset_flag(true);
              next_bs2.reset_flag(false);
              next_bs2.open_flag_e = bs.hand_s[0];
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
      } else if(i + 1 == 7) {
        next_bs.lt5_flag_e = true;

        // 7 を使った相手は、使用後も 5 未満のカードを手札に残している必要がある。
        // deck_or_hand_e() に 7 自身が存在するだけでは、この枝は成立しない。
        bool has_remaining_hand_candidate = false;
        for(int j = 0; j < 8; j++) {
          if(next_bs.hand_e(j)) {
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

std::pair<bool, int> draw_win(const belief_state& bs) {
  CW_BUMP(draw_win);
  auto t = is_terminated_win(bs);
  if(t.first != -1) return t;

  bool all_true = true;
  int max_f = -1;

  // open_e() はこのループの中で不変 (bs は const 参照) なので括り出す。
  const int open_card = bs.open_e();
  for(int i = 0; i < 8; i++) {
    if(!all_true) break;
    if(bs.deck(i, open_card)) {
      belief_state next_bs = draw(bs, i + 1);
      next_bs.barrier_s = false;
      next_bs.is_my_turn = !next_bs.is_my_turn;
      if(commentable_bs) std::cout << next_bs.count_deck() << "draw " << i + 1 << std::endl;

      // --- 自分の手札の選択 (ORノード) ---
      auto res0 = use_win(next_bs, next_bs.hand_s[0]);

      bool or_first = false;
      int or_second = 0;

      // 手札の2枚が違うカードなら、もう一方も評価する
      if(next_bs.hand_s[0] != i + 1) {
        auto res1 = use_win(next_bs, next_bs.hand_s[1]);

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

std::pair<bool, int> sol_win(const belief_state& bs, int card) {
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

std::pair<bool, int> wiz_win(const belief_state& bs, bool to0p) {
  CW_BUMP(wiz_win);
  if(!bs.is_wiz_choice) exit_with_print(bs, "wiz_win called when not in wiz_choice state");
  if(!to0p && bs.open_e() == 8 && !bs.barrier_e) return {true, 1};
  if(to0p && bs.hand_s[0] == 7) return {false, 0};
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

std::vector<int> able_actions(const belief_state& bs, int card, bool is_second_player) {
  int base = 40 + card - 1;
  std::vector<int> actions;

  if(bs.hand_s[1] == 0 && bs.is_wiz_choice) {
    // --- 自分を対象とする場合 ---
    if((card == 0 && !is_second_player) || (card == 1 && is_second_player)) {
      for(int j = 0; j < 8; j++)
        if(bs.deck(j)) actions.push_back(44000 + is_second_player * 100 + (bs.other_hand_s(card) - 1) * 10 + j);
    }
    // --- 相手を対象とする場合 ---
    else if((card == 0 && is_second_player) || (card == 1 && !is_second_player)) {
      if(!bs.barrier_e) {
        for(int i = 0; i < 7; i++) {
          if(bs.hand_e(i)) { // 相手が捨てさせられるカード
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
      if(bs.hand_e(i)) actions.push_back(base * 10 + i);
  } else if(card == 3) {
    // 相手の判明するカード
    int other_i = bs.other_hand_s(card) - 1;
    if(bs.hand_e(other_i)) actions.push_back(base * 100 + other_i * 11);
  } else if(card == 6) {
    // 相手と交換するカード
    for(int i = 0; i < 8; i++)
      if(bs.hand_e(i)) actions.push_back(base * 100 + (bs.other_hand_s(card) - 1) * 10 + i);
  }

  return actions;
}

int action_count(const belief_state& bs) {
  if(bs.is_sol_choice) {
    int count = 0;
    for(int i = 1; i < 8; i++) {
      if(bs.deck_or_hand_e(i)) count++;
    }
    return count;
  }
  if(bs.is_wiz_choice) return 2;

  if(bs.hand_s[1] == 0) return 0;
  return bs.hand_s[0] == bs.hand_s[1] ? 1 : 2;
}
#endif
