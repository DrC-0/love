#ifndef BELIEF_STATE_WIN_HPP
#define BELIEF_STATE_WIN_HPP
#include "belief_state.hpp"
#include <unordered_map>

// 必勝が成立するか、成立までにプレイヤーの行動が何回続くか。
// turns は is_win が真のときだけ意味を持つ。偽のときは 0 を入れる。
struct win_result {
  bool is_win;
  int turns;
};

// 必敗側。win_result と同じ形だが別の型にして、取り違えをコンパイル時に止める。
// use_lose は win.draw_win の結果を受けるので、混ざる余地が実際にある。
struct lose_result {
  bool is_lose;
  int turns;
};

// 意思決定点の種類。行動番号 0〜7 の読み方がこれで決まる。
enum class decision_kind {
  play_card, // 行動番号 = カード − 1 (0〜7 が カード 1〜8)
  wizard_target, // 0 = 自分, 1 = 相手
  soldier_declaration, // 0 = 宣言なし, 1〜7 = カード 2〜8
};

// どの行動で必勝が成立するか。bit i が行動番号 i に対応する。
struct win_decision {
  decision_kind kind;
  unsigned char win_bits; // 行動 i で必勝なら bit i が立つ
  unsigned char turns[8]; // bit i が立っているときだけ意味を持つ

  bool has_win() const {
    return win_bits != 0;
  }
  bool wins_with(int action) const {
    return (win_bits >> action) & 1;
  }
};

// 終端判定。カードを問わない位置レベルの判定だけを見る。
// not_terminal = まだ決着していない、lost = ルール上すでに負け、
// won = 山札が尽きて手札比較で勝ち。won のとき手札は1枚なので、
// どのカードで勝つかを言う必要が無い (だから戻り値はこの3値だけでよい)。
enum class terminal_kind { not_terminal,
                           lost,
                           won };

terminal_kind check_terminal(const belief_state& bs);
std::vector<int> able_actions(const belief_state& bs, int card, bool is_second_player);
int action_count(const belief_state& bs);

// belief_state と引数以外の可変状態を読まない純関数なので、6本はメモ化する。
// メモ表は判定ごとに分ける (関数が違えば同じ (bs, extra) でも答えが違うため)。
struct belief_state_win_checker {
  // belief_state の16フィールドと追加の鍵 (card 0..8 / 魔術師の対象) を 59bit に詰める。
  // 値域: open_flag / sol_flag / hand_s は 0..8、trash[i] は max_num[i] 以下。
  static unsigned long long key(const belief_state& bs, int extra);

  // 意思決定点で、自分のどの行動が必勝かを返す。draw_win から再帰的に呼ばれる
  // ため、他の 5 本と同じくメモ化する。
  win_decision is_win(const belief_state& bs);
  win_result use_win(const belief_state& bs, Card card);
  win_result enemy_turn_win(const belief_state& bs);
  win_result draw_win(const belief_state& bs);
  win_result soldier_win(const belief_state& bs, Card card);
  win_result wizard_win(const belief_state& bs, bool to_self);

private:
  win_decision is_win_uncached(const belief_state& bs);
  win_result use_win_uncached(const belief_state& bs, Card card);
  win_result enemy_turn_win_uncached(const belief_state& bs);
  win_result draw_win_uncached(const belief_state& bs);
  win_result soldier_win_uncached(const belief_state& bs, Card card);
  win_result wizard_win_uncached(const belief_state& bs, bool to_self);

  std::unordered_map<unsigned long long, win_decision> m_is_win;
  std::unordered_map<unsigned long long, win_result> m_use_win, m_enemy_turn_win,
      m_draw_win, m_soldier_win, m_wizard_win;
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

terminal_kind check_terminal(const belief_state& bs) {
  CW_BUMP(check_terminal);
  // 大臣(7) を持っていて手札の合計が12以上ならルール上の負け。
  if(bs.have_s(Card{7}) && bs.hand_s[1].has_value() && bs.hand_s[0].value().value() + bs.hand_s[1].value().value() >= 12) {
    return terminal_kind::lost;
  }
  // 山札が尽きたら手札の大きい方が勝ち。ここは手札1枚なので勝ちカードは自明。
  // count_deck() は8要素ループなので、スカラの比較3つを先に評価する。
  // どれも副作用が無いので && の順序を入れ替えても意味は変わらない。
  if(!bs.hand_s[1].has_value() && !bs.is_wiz_choice && !bs.is_sol_choice && bs.count_deck() < 2) {
    MaybeCard max = bs.hand_e_max();
    if(max.value() < bs.hand_s[0].value()) return terminal_kind::won;
    else return terminal_kind::lost;
  }
  // 兵士(1) / 騎士(3) / 魔術師(5) を出した瞬間に勝つ判定は、どのカードを出すかが
  // 決まってからの問いなので use_win_uncached の先頭に移した。
  return terminal_kind::not_terminal;
}

win_decision belief_state_win_checker::is_win(const belief_state& bs) {
  const unsigned long long k = key(bs, 0);
  if(!commentable_bs) {
    auto it = m_is_win.find(k);
    if(it != m_is_win.end()) return it->second;
  }
  auto r = is_win_uncached(bs);
  if(!commentable_bs) m_is_win.emplace(k, r);
  return r;
}

win_decision belief_state_win_checker::is_win_uncached(const belief_state& bs) {
  // 兵士の宣言・魔術師の対象選択・手札2枚は、どれも自分の意思決定点なので
  // is_my_turn が真のときにしか立たない。また宣言と対象選択は同時に立たない。
  if(!bs.is_my_turn && (bs.is_sol_choice || bs.is_wiz_choice || bs.hand_s[1].has_value()))
    exit_with_print(bs, "is_win: 意思決定点なのに is_my_turn が偽");
  if(bs.is_sol_choice && bs.is_wiz_choice)
    exit_with_print(bs, "is_win: 宣言ノードと対象選択ノードが同時に立っている");
  // 決着していれば選べる行動は無い。won は山札が尽きた状態 (残り1枚は開始時に
  // 伏せるカードなので引けない) で、手札1枚のままもう行動できないから、
  // 「どの行動で勝つか」を答える is_win にとっては lost と同じく空になる。
  // なおこの分岐は現状どこからも到達しない (is_win を呼ぶ3箇所はいずれも
  // 手札2枚か、決着していない情報集合のみ。win 446 の 6,298,972 件すべてが
  // not_terminal だった)。
  if(check_terminal(bs) != terminal_kind::not_terminal) {
    return win_decision{decision_kind::play_card, 0, {}};
  }

  if(bs.is_my_turn && !bs.hand_s[1].has_value() && bs.is_sol_choice) {
    win_decision d{decision_kind::soldier_declaration, 0, {}};
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "soldier");
    for(int c = 2; c <= 8; c++) {
      Card card{c};
      if(bs.hand_e(card)) {
        auto res = soldier_win(bs, card);
        // 行動番号: 0 = 宣言なし、1〜7 = カード 2〜8
        if(res.is_win) {
          const int action = c - 1;
          d.win_bits |= (unsigned char)(1u << action);
          d.turns[action] = (unsigned char)(res.turns + 1);
        }
      }
    }
    return d;

  } else if(bs.is_my_turn && !bs.hand_s[1].has_value() && bs.is_wiz_choice) {
    win_decision d{decision_kind::wizard_target, 0, {}};
    auto res_self = wizard_win(bs, true);
    auto res_enemy = wizard_win(bs, false);
    if(res_self.is_win) {
      d.win_bits |= 1u << 0;
      d.turns[0] = (unsigned char)(res_self.turns + 1);
    }
    if(res_enemy.is_win) {
      d.win_bits |= 1u << 1;
      d.turns[1] = (unsigned char)(res_enemy.turns + 1);
    }
    return d;

  } else if(bs.is_my_turn && bs.hand_s[1].has_value()) {
    // 手札が2枚あるのは、引いた直後に「どちらを出すか」を選ぶ意思決定点だけ。
    win_decision d{decision_kind::play_card, 0, {}};
    // 手札の2枚が同じカードなら片方だけ評価する (無駄を省く)。
    // 行動番号はカード − 1 なので、同じカードなら同じビットになる。
    const Card c0 = bs.hand_s[0].value();
    auto res0 = use_win(bs, c0);
    if(res0.is_win) {
      d.win_bits |= (unsigned char)(1u << c0.index());
      d.turns[c0.index()] = (unsigned char)res0.turns;
    }
    if(bs.hand_s[0] != bs.hand_s[1]) {
      const Card c1 = bs.hand_s[1].value();
      auto res1 = use_win(bs, c1);
      if(res1.is_win) {
        d.win_bits |= (unsigned char)(1u << c1.index());
        d.turns[c1.index()] = (unsigned char)res1.turns;
      }
    }
    return d;
  }
  // 手札が1枚で選択ノードでもない = 相手の手番待ち。自分の行動は無い。
  return win_decision{decision_kind::play_card, 0, {}};
}

win_result belief_state_win_checker::use_win(const belief_state& bs, Card card) {
  const unsigned long long k = key(bs, card.value());
  if(!commentable_bs) {
    auto it = m_use_win.find(k);
    if(it != m_use_win.end()) return it->second;
  }
  auto r = use_win_uncached(bs, card);
  if(!commentable_bs) m_use_win.emplace(k, r);
  return r;
}

win_result belief_state_win_checker::use_win_uncached(const belief_state& bs, Card card) {
  CW_BUMP(use_win);
  // 自分がカードを出す局面なので、必ず自分の手番。
  if(!bs.is_my_turn) exit_with_print(bs, "use_win: is_my_turn が偽");

  // そのカードを出した瞬間に勝ちが決まる3つ。旧 check_terminal の第3ブロックで、
  // 出すカードが決まってからの問いなのでここにある。他の分岐より先に見る。
  if(!bs.barrier_e && bs.hand_s[1].has_value()) {
    if(card == Card{1}) {
      MaybeCard oe = bs.open_e();
      if(oe.has_value() && oe.value() != Card{1}) return {true, 1};
    }
    if(card == Card{3} && bs.hand_e_max().value() < bs.other_hand_s(Card{3}).value()) {
      return {true, 1};
    }
    if(card == Card{5} && bs.open_e() == Card{8}) {
      return {true, 1};
    }
  }

  if(card == Card{3} && !bs.barrier_s && bs.hand_e_min().value() > bs.other_hand_s(Card{3}).value()) {
    return {false, 0};
  }
  if(card == Card{8}) return {false, 0};
  if(commentable_bs) std::cout << bs.count_deck() << "use :" << card.value() << std::endl;

  // is_my_turn はここでは触らない (自分の手番のまま)。相手の手番に移すのは
  // enemy_turn_win を呼ぶ直前で、カードごとに行う。兵士(1) と魔術師(5) は
  // 選択ノードを挟むので soldier_win / wizard_win の側で移す。
  struct belief_state next_bs = bs;
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
    next_bs.is_my_turn = false;
    auto res = enemy_turn_win(next_bs);
    return {res.is_win, res.is_win ? res.turns + 1 : 0};
  } else if(card == Card{1}) {
    next_bs.is_sol_choice = true;
    bool has_true = false;
    int min_t = 1e9;
    // カード1は宣言対象外だが、候補として残っていても異常ではない。
    validate_hand_e_candidate(bs, "sol");
    for(int c2 = 2; c2 <= 8; c2++) {
      Card card2{c2};
      if(bs.hand_e(card2)) {
        auto res = soldier_win(next_bs, card2);
        if(res.is_win) {
          has_true = true;
          min_t = std::min(min_t, res.turns);
        }
      }
    }
    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == Card{2}) {
    next_bs.is_my_turn = false;
    bool all_true = true;
    int max_f = -1;

    for(int c2 = 1; c2 <= 8; c2++) {
      Card card2{c2};
      if(all_true)
        if(bs.hand_e(card2)) {
          struct belief_state next_bs2 = next_bs;
          next_bs2.open_flag_e = card2;
          auto res = enemy_turn_win(next_bs2);
          if(res.is_win) {
            max_f = std::max(max_f, res.turns);
          } else {
            all_true = false;
          }
        }
    }
    if(max_f == -1) return {false, 0};
    return {all_true, all_true ? max_f + 1 : 0};
  } else if(card == Card{3}) {
    next_bs.is_my_turn = false;
    Card other = bs.other_hand_s(Card{3}).value();
    if(bs.hand_e(other)) {
      struct belief_state next_bs2 = next_bs;
      next_bs2.open_flag_e = other;
      // next_bs2.open_flag_s = other;//自分のフラグは不要
      auto res = enemy_turn_win(next_bs2);
      return {res.is_win, res.is_win ? res.turns + 1 : 0};
    }
    return {false, 0};
  } else if(card == Card{4}) {
    next_bs.barrier_s = true;
    next_bs.is_my_turn = false;
    auto res = enemy_turn_win(next_bs);
    return {res.is_win, res.is_win ? res.turns + 1 : 0};
  } else if(card == Card{5}) {
    next_bs.is_wiz_choice = true;
    auto res_self = wizard_win(next_bs, true);
    auto res_enemy = wizard_win(next_bs, false);

    bool has_true = res_self.is_win || res_enemy.is_win;

    int min_t = 1e9;
    if(res_self.is_win) min_t = std::min(min_t, res_self.turns);
    if(res_enemy.is_win) min_t = std::min(min_t, res_enemy.turns);

    return {has_true, has_true ? min_t + 1 : 0};
  } else if(card == Card{6}) {
    next_bs.is_my_turn = false;
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
        if(res.is_win) {
          max_f = std::max(max_f, res.turns);
        } else {
          all_true = false;
        }
      }
    }
    if(max_f == -1) return {false, 0};
    return {all_true, all_true ? max_f + 1 : 0};
  } else if(card == Card{7}) {
    // next_bs.lt5_flag_s = true;//自分のフラグは不要
    next_bs.is_my_turn = false;
    auto res = enemy_turn_win(next_bs);
    return {res.is_win, res.is_win ? res.turns + 1 : 0};
  } else return {false, 0};
}

win_result belief_state_win_checker::enemy_turn_win(const belief_state& bs) {
  const unsigned long long k = key(bs, 0);
  if(!commentable_bs) {
    auto it = m_enemy_turn_win.find(k);
    if(it != m_enemy_turn_win.end()) return it->second;
  }
  auto r = enemy_turn_win_uncached(bs);
  if(!commentable_bs) m_enemy_turn_win.emplace(k, r);
  return r;
}

win_result belief_state_win_checker::enemy_turn_win_uncached(const belief_state& bs) {
  CW_BUMP(enemy_turn_win);
  // 相手がカードを出す局面なので、必ず相手の手番。
  if(bs.is_my_turn) exit_with_print(bs, "enemy_turn_win: is_my_turn が真");
  const terminal_kind t = check_terminal(bs);
  if(t != terminal_kind::not_terminal) return {t == terminal_kind::won, 0};

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
      // is_my_turn はここでは触らない。入口で偽 (相手の手番) だと確かめてあり、
      // カード5の ef_wizard はそれを見て「相手が使う」枝に入る。
      // 自分の手番に戻すのは draw_win を呼ぶ直前で、カードごとに行う。
      next_bs = reset_flag_by_use(next_bs, false, card);

      if(card == Card{5}) next_bs.not7_flag_e = true;

      // 3. 各カードごとの再帰評価
      if(card == Card{3}) {
        if(!bs.barrier_s) next_bs.open_flag_e = bs.hand_s[0].value();
        next_bs.is_my_turn = true; // 相手が出し終えたので自分が引く
        auto res = draw_win(next_bs);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else all_true = false;
      } else if(card == Card{4}) {
        next_bs.barrier_e = true;
        next_bs.is_my_turn = true; // 相手が出し終えたので自分が引く
        auto res = draw_win(next_bs);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else all_true = false;
      } else if(card == Card{5}) {
        if(bs.open_e() == Card{7}) continue;

        // 魔術師専用の集約ラムダ
        auto eval_wiz_preds = [&](const ef_wizard_preds& preds) -> win_result {
          if(preds.empty()) return {false, 0}; // 空なら敗北(深さ0)扱い
          int local_max_f = -1;
          bool local_all_true = true;
          for(const auto& p : preds) {
            auto res = draw_win(p);
            if(res.is_win) local_max_f = std::max(local_max_f, res.turns);
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

        if(res_self.is_win && res_enemy.is_win) max_f = std::max(res_self.turns, res_enemy.turns);
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
              next_bs2.is_my_turn = true; // 相手が出し終えたので自分が引く
              auto res = draw_win(next_bs2);
              if(res.is_win) gene_max_f = std::max(gene_max_f, res.turns);
              else gene_all_true = false;
            }
          }
          if(gene_all_true && gene_max_f != -1) max_f = std::max(max_f, gene_max_f);
          else all_true = false;
        } else {
          next_bs.not7_flag_e = true;
          next_bs.is_my_turn = true; // 相手が出し終えたので自分が引く
          auto res = draw_win(next_bs);
          if(res.is_win) max_f = std::max(max_f, res.turns);
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

        next_bs.is_my_turn = true; // 相手が出し終えたので自分が引く
        auto res = draw_win(next_bs);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else all_true = false;
      } else {
        next_bs.is_my_turn = true; // 相手が出し終えたので自分が引く
        auto res = draw_win(next_bs);
        if(res.is_win) max_f = std::max(max_f, res.turns);
        else all_true = false;
      }
    }
  }

  // 指定された win に応じて最適な深さを +1 して返す
  return {all_true, all_true ? max_f + 1 : 0};
}

win_result belief_state_win_checker::draw_win(const belief_state& bs) {
  const unsigned long long k = key(bs, 0);
  if(!commentable_bs) {
    auto it = m_draw_win.find(k);
    if(it != m_draw_win.end()) return it->second;
  }
  auto r = draw_win_uncached(bs);
  if(!commentable_bs) m_draw_win.emplace(k, r);
  return r;
}

win_result belief_state_win_checker::draw_win_uncached(const belief_state& bs) {
  CW_BUMP(draw_win);
  // 山札から引くのは自分。引いた2枚からどちらを出すかを選ぶところまでが
  // 自分の手番なので、入口では真でなければならない。
  if(!bs.is_my_turn) exit_with_print(bs, "draw_win: is_my_turn が偽");
  const terminal_kind t = check_terminal(bs);
  if(t != terminal_kind::not_terminal) return {t == terminal_kind::won, 0};

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
      // 引いた本人が2枚から選ぶ局面なので自分の手番。反転ではなく true を入れる。
      next_bs.is_my_turn = true;
      if(commentable_bs) std::cout << next_bs.count_deck() << "draw " << card.value() << std::endl;

      // --- 自分の手札の選択 (ORノード) ---
      // 「この局面で自分のどの行動が勝つか」はそのまま is_win の問い。
      // 終端判定も is_win の中で捌かれるので、ここで先に見る必要はない。
      const win_decision d = is_win(next_bs);
      const bool or_first = d.has_win();

      // --- 山札ドローの集約 (ANDノード) ---
      if(or_first) {
        int or_second = 255;
        for(int action = 0; action < 8; action++) {
          if(d.wins_with(action)) or_second = std::min(or_second, (int)d.turns[action]);
        }
        max_f = std::max(max_f, or_second);
      } else {
        all_true = false;
      }
    }
  }

  if(max_f == -1 || !all_true) return {false, 0};

  return {true, max_f};
}

win_result belief_state_win_checker::soldier_win(const belief_state& bs, Card card) {
  const unsigned long long k = key(bs, card.value());
  if(!commentable_bs) {
    auto it = m_soldier_win.find(k);
    if(it != m_soldier_win.end()) return it->second;
  }
  auto r = soldier_win_uncached(bs, card);
  if(!commentable_bs) m_soldier_win.emplace(k, r);
  return r;
}

win_result belief_state_win_checker::soldier_win_uncached(const belief_state& bs, Card card) {
  CW_BUMP(soldier_win);
  // 兵士の宣言をするのは自分。
  if(!bs.is_my_turn) exit_with_print(bs, "soldier_win: is_my_turn が偽");
  if(!bs.is_sol_choice) exit_with_print(bs, "soldier_win called when not in sol_choice state");
  if(bs.open_e() == card && !bs.barrier_e) return {true, 1};
  struct belief_state next_bs = bs;
  // 宣言が確定したので、保留中の兵士選択を解除する。
  next_bs.is_sol_choice = false;
  next_bs.add_sol_e(card);
  // 宣言まで済んだので相手の手番に移す (use_win は触らない)。
  next_bs.is_my_turn = false;
  auto res = enemy_turn_win(next_bs);
  return {res.is_win, res.is_win ? res.turns + 1 : 0};
}

win_result belief_state_win_checker::wizard_win(const belief_state& bs, bool to_self) {
  const unsigned long long k = key(bs, to_self ? 1 : 0);
  if(!commentable_bs) {
    auto it = m_wizard_win.find(k);
    if(it != m_wizard_win.end()) return it->second;
  }
  auto r = wizard_win_uncached(bs, to_self);
  if(!commentable_bs) m_wizard_win.emplace(k, r);
  return r;
}

win_result belief_state_win_checker::wizard_win_uncached(const belief_state& bs, bool to_self) {
  CW_BUMP(wizard_win);
  // 魔術師を使うのは自分。ef_wizard がこのフラグで枝を分ける。
  if(!bs.is_my_turn) exit_with_print(bs, "wizard_win: is_my_turn が偽");
  if(!bs.is_wiz_choice) exit_with_print(bs, "wizard_win called when not in wiz_choice state");
  if(!to_self && bs.open_e() == Card{8} && !bs.barrier_e) return {true, 1};
  if(to_self && bs.hand_s[0] == Card{7}) return {false, 0};
  struct belief_state next_bs = bs;
  next_bs.is_wiz_choice = false;
  ef_wizard_preds preds;
  ef_wizard(next_bs, to_self, preds);
  if(preds.empty()) return {false, 0};

  int local_max_f = -1;
  bool local_all_true = true;
  for(const auto& p : preds) {
    auto res = enemy_turn_win(p);
    if(!res.is_win) {
      // ANDノードなので1つでも偽なら結果は決まる。残りは見ない。
      local_all_true = false;
      break;
    }
    local_max_f = std::max(local_max_f, res.turns);
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
