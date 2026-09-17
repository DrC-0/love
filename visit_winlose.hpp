#ifndef VISIT_WINLOSE_HPP
#define VISIT_WINLOSE_HPP

#include "belief_state_history.hpp"
#include "belief_state_lose.hpp"
#include "analysis_points.hpp"
#include <cassert>
#include <map>
#include <string>
#include <utility>

// cfr_org.cpp が定義するグローバル統計。他の展開部分 8 本と共有しているため
// 廃止せず、判定器から更新する。

// 必勝・必敗判定と統計を行う判定器。
// 再帰1段ぶんの退避は node::depth を添字にした固定長配列に置き、このクラスに閉じる。
// depth は node::do_action で ++、node::undo_action で -- され、上限は 30
// （loveletter.hpp の npi[3][32] が根拠）。
struct winlose_visitor {
  // 判定は belief_state と引数以外の可変状態を読まない純関数なので、
  // このインスタンスの生存期間 (部分ゲーム1回の実行) を通してメモ化を共有する。
  belief_state_win_checker wc;
  belief_state_lose_checker lc{wc};

  // 削減対象マーカー。bool ではなく int カウンタ。
  // 祖先が立てた分と自分が立てた分を区別せずに済むので con_cutting_* の退避が要らない。
  int cutting_w = 0;
  int cutting_l = 0;

  // node::depth の上限。実測では 6 つの部分ゲーム (557 / 111 / 446 / 228 / 118 /
  // 567 / 123) で最大 25〜28 だった。loveletter.hpp の npi[3][32] と
  // node::do_action の npi[turn][depth + 1] から理論上限は 30。
  // メモリは制約ではない (cfrorg のピーク RSS は 3.6 MB 一定) ので余裕を取る。
  static constexpr int MAX_DEPTH = 64;

  // org_ds_draw 用
  struct play_frame {
    bool w_inc; // enter_play で cutting_w に足した分
    bool l0; // 分岐0で cutting_l に足す分 (res_0lose.first)
    bool l1; // 分岐1で cutting_l に足す分 (res_1lose.first)
  };
  play_frame pf[MAX_DEPTH]{};

  // org_ds_play case 1（兵士）用
  bool soldier_inc[MAX_DEPTH]{}; // 直前の enter_soldier が返した res_win.first
  // belief_state は (部分ゲームの3枚, 履歴キー, org/rnd) だけで決まり、前2つは
  // 実行を通して不変なので、履歴キーが同じなら作り直す必要がない。enter_soldier は
  // 同じ意思決定点で宣言カードの候補ごとに呼ばれるため、2回目以降が命中する。
  // 再帰は深さ n.depth + 1 以降のスロットを使うので、このスロットを壊さない。
  std::string soldier_key[MAX_DEPTH]{};
  belief_state soldier_bs[MAX_DEPTH]{};

  // org_ds_play case 5（魔術師）用
  struct wizard_frame {
    // 添字 0 = 相手に使う (to_self == false)、1 = 自分に使う (to_self == true)。
    // enter_wizard が wizard_win(bs, false) を [0] に入れるのと合わせてある。
    bool win[2];
    bool lose[2];
  };
  wizard_frame wf[MAX_DEPTH]{};

  std::map<std::string, infset>::iterator infset_for(node &) {
    static auto dummy_it = table_infset.emplace("__dummy__", infset{}).first;
    return dummy_it;
  }

  void on_chance_points() {
    rand_points++;
  }
  void on_terminal_points() {
    end_points++;
  }
  void on_decision_points(int turn) {
    if(turn == 0) p1_points++;
    else p2_points++;
  }

  // ---- org_ds_draw ----
  void enter_play(node &n, int c1, int c2) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    belief_state bs(n.open, key, false);
    // 行動ごとの必勝は is_win がまとめて返す。use_win を直に呼ぶと終端局面の
    // 扱いを自前で書くことになるので呼ばない。
    const win_decision dwin = wc.is_win(bs);
    const win_result res_0win{dwin.wins_with(c1 - 1), dwin.turns[c1 - 1]};
    const win_result res_1win{dwin.wins_with(c2 - 1), dwin.turns[c2 - 1]};
    auto res_0lose = lc.use_lose(bs, Card{c1});
    auto res_1lose = lc.use_lose(bs, Card{c2});
    bool rm_bywin = res_0win.is_win || res_1win.is_win || cutting_w > 0;
    bool rm_bylose = res_0lose.is_lose || res_1lose.is_lose || cutting_l > 0;
    assert(0 <= res_0win.turns && res_0win.turns < 11);
    assert(0 <= res_1win.turns && res_1win.turns < 11);

    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_0win.is_win && res_1win.is_win) {
      if(res_0win.turns <= res_1win.turns) win_points[res_0win.turns]++;
      else win_points[res_1win.turns]++;
    } else if(res_0win.is_win) win_points[res_0win.turns]++;
    else if(res_1win.is_win) win_points[res_1win.turns]++;
    else win_points[0]++;
    if(res_0lose.is_lose && res_1lose.is_lose) {
      if(res_0lose.turns >= res_1lose.turns) lose_points[res_0lose.turns]++;
      else lose_points[res_1lose.turns]++;
    } else if(res_0lose.is_lose) lose_points[res_0lose.turns]++;
    else if(res_1lose.is_lose) lose_points[res_1lose.turns]++;
    else lose_points[0]++;

    pf[n.depth] = {res_0win.is_win || res_1win.is_win, res_0lose.is_lose, res_1lose.is_lose};
    cutting_w += pf[n.depth].w_inc;
    cutting_l += pf[n.depth].l0;
  }
  void mid_play(const node &n) {
    cutting_l -= pf[n.depth].l0;
    cutting_l += pf[n.depth].l1;
  }
  void leave_play(const node &n) {
    cutting_l -= pf[n.depth].l1;
    cutting_w -= pf[n.depth].w_inc;
  }

  // ---- org_ds_play case 1（兵士） ----
  // 宣言カード i ごとに呼ばれる。belief_state は候補ごとに作り直しになるが、
  // enter_wizard と同じ形にして読みやすさを優先する。
  void enter_soldier(node &n, int i) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    if(soldier_key[n.depth] != key) {
      soldier_key[n.depth] = key;
      soldier_bs[n.depth] = belief_state(n.open, key, false);
    }
    auto res_win = wc.soldier_win(soldier_bs[n.depth], Card{i});
    bool rm_bywin = res_win.is_win || cutting_w > 0;
    bool rm_bylose = cutting_l > 0;
    assert(0 <= res_win.turns && res_win.turns < 11);
    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_win.is_win) win_points[res_win.turns]++;
    else win_points[0]++;
    // 兵士の宣言ノードには必敗判定が無いので、全件が「該当なし」= index 0 になる。
    // これを数えないと lose_points の合計が decision_points[0] と一致しない。
    lose_points[0]++;
    soldier_inc[n.depth] = res_win.is_win;
  }
  void enter_soldier_branch(const node &n) {
    cutting_w += soldier_inc[n.depth];
  }
  void leave_soldier_branch(const node &n) {
    cutting_w -= soldier_inc[n.depth];
  }

  // ---- org_ds_play case 5（魔術師） ----
  void enter_wizard(node &n) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    belief_state bs(n.open, key, false);
    // 改名前の wc.wiz_win(bs, 0) は「相手」、wc.wiz_win(bs, 1) は「自分」だった。
    // res_0win が相手、res_1win が自分という順序 (is_lose の 0/1 とは逆)。
    // enter_wizard の集約は res_0* / res_1* について対称 (真偽は OR、深さは
    // min/max) なので入れ替えても出力は変わらないが、0/1 の規約整理は
    // is_lose の多目的コードとまとめて別の変更で行うため、ここでは変えない。
    auto res_0win = wc.wizard_win(bs, false);
    auto res_1win = wc.wizard_win(bs, true);
    auto res_0lose = lc.wizard_lose(bs, false);
    auto res_1lose = lc.wizard_lose(bs, true);
    bool rm_bywin = res_0win.is_win || res_1win.is_win || cutting_w > 0;
    bool rm_bylose = res_0lose.is_lose || res_1lose.is_lose || cutting_l > 0;
    assert(0 <= res_0win.turns && res_0win.turns < 11);
    assert(0 <= res_1win.turns && res_1win.turns < 11);

    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_0win.is_win && res_1win.is_win) {
      if(res_0win.turns <= res_1win.turns) win_points[res_0win.turns]++;
      else win_points[res_1win.turns]++;
    } else if(res_0win.is_win) win_points[res_0win.turns]++;
    else if(res_1win.is_win) win_points[res_1win.turns]++;
    else win_points[0]++;
    if(res_0lose.is_lose && res_1lose.is_lose) {
      if(res_0lose.turns >= res_1lose.turns) lose_points[res_0lose.turns]++;
      else lose_points[res_1lose.turns]++;
    } else if(res_0lose.is_lose) lose_points[res_0lose.turns]++;
    else if(res_1lose.is_lose) lose_points[res_1lose.turns]++;
    else lose_points[0]++;

    wf[n.depth] = {{res_0win.is_win, res_1win.is_win},
                   {res_0lose.is_lose, res_1lose.is_lose}};
  }
  // to_self == false: 相手に使う分岐 (do_action 6) = 添字 0
  // to_self == true : 自分に使う分岐 (do_action 7) = 添字 1
  // to_self も bs も「手を打つプレイヤーから見た自分/相手」なので、
  // ここに絶対的なプレイヤ番号を持ち込む必要は無い。
  void enter_wizard_branch(const node &n, bool to_self) {
    const int k = to_self ? 1 : 0;
    cutting_w += wf[n.depth].win[k];
    cutting_l += wf[n.depth].lose[k];
  }
  void leave_wizard_branch(const node &n, bool to_self) {
    const int k = to_self ? 1 : 0;
    cutting_w -= wf[n.depth].win[k];
    cutting_l -= wf[n.depth].lose[k];
  }
};

#endif
