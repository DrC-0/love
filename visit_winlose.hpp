#ifndef VISIT_WINLOSE_HPP
#define VISIT_WINLOSE_HPP

#ifndef BF_POSITION_HPP
#include "bf_position.hpp"
#endif
#include <cassert>
#include <map>
#include <string>
#include <utility>

// cfr_org.cpp が定義するグローバル統計。他の展開部分 8 本と共有しているため
// 廃止せず、判定器から更新する。
extern unsigned long int p1_points;
extern unsigned long int p2_points;
extern unsigned long int rand_points;
extern unsigned long int end_points;
extern unsigned long int win_points[11];
extern unsigned long int lose_points[11];
extern unsigned long int decision_points[4];
extern std::map<std::string, infset> table_infset;

// 必勝・必敗判定と統計を行う判定器。
// 再帰1段ぶんの退避は node::depth を添字にした固定長配列に置き、このクラスに閉じる。
// depth は node::do_action で ++、node::undo_action で -- され、上限は 30
// （loveletter.hpp の npi[3][32] が根拠）。
struct winlose_visitor {
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
  bf_position sol_bfp[MAX_DEPTH]{};
  bool sol_inc[MAX_DEPTH]{}; // 直前の on_guess が返した res_win.first

  // org_ds_play case 5（魔術師）用
  struct wiz_frame {
    bool win[2]; // {res_0win.first, res_1win.first}
    bool lose[2]; // {res_0lose.first, res_1lose.first}
    bool is_zero;
  };
  wiz_frame wf[MAX_DEPTH]{};

  std::map<std::string, infset>::iterator infset_for(node &) {
    static auto dummy_it = table_infset.emplace("__dummy__", infset{}).first;
    return dummy_it;
  }

  void on_chance() {
    rand_points++;
  }
  void on_terminal() {
    end_points++;
  }
  void on_decision(int turn) {
    if(turn == 0) p1_points++;
    else p2_points++;
  }

  // ---- org_ds_draw ----
  void enter_play(node &n, int c1, int c2) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    bf_position bfp(n.open, key, false);
    auto res_0win = use_win(bfp, c1);
    auto res_1win = use_win(bfp, c2);
    auto res_0lose = use_lose(bfp, c1);
    auto res_1lose = use_lose(bfp, c2);
    bool rm_bywin = res_0win.first || res_1win.first || cutting_w > 0;
    bool rm_bylose = res_0lose.first || res_1lose.first || cutting_l > 0;
    assert(0 <= res_0win.second && res_0win.second < 11);
    assert(0 <= res_1win.second && res_1win.second < 11);

    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_0win.first && res_1win.first) {
      if(res_0win.second <= res_1win.second) win_points[res_0win.second]++;
      else win_points[res_1win.second]++;
    } else if(res_0win.first) win_points[res_0win.second]++;
    else if(res_1win.first) win_points[res_1win.second]++;
    else win_points[0]++;
    if(res_0lose.first && res_1lose.first) {
      if(res_0lose.second >= res_1lose.second) lose_points[res_0lose.second]++;
      else lose_points[res_1lose.second]++;
    } else if(res_0lose.first) lose_points[res_0lose.second]++;
    else if(res_1lose.first) lose_points[res_1lose.second]++;
    else lose_points[0]++;

    pf[n.depth] = {res_0win.first || res_1win.first, res_0lose.first, res_1lose.first};
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
  void enter_soldier(node &n) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    sol_bfp[n.depth] = bf_position(n.open, key, false);
  }
  void on_guess(const node &n, int i) {
    auto res_win = sol_win(sol_bfp[n.depth], i);
    bool rm_bywin = res_win.first || cutting_w > 0;
    bool rm_bylose = cutting_l > 0;
    assert(0 <= res_win.second && res_win.second < 11);
    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_win.first) win_points[res_win.second]++;
    else win_points[0]++;
    sol_inc[n.depth] = res_win.first;
  }
  void enter_guess(const node &n) {
    cutting_w += sol_inc[n.depth];
  }
  void leave_guess(const node &n) {
    cutting_w -= sol_inc[n.depth];
  }

  // ---- org_ds_play case 5（魔術師） ----
  void enter_wizard(node &n) {
    std::string key = n.org_his_p[n.turn].get_hash_value();
    bf_position bfp(n.open, key, false);
    auto res_0win = wiz_win(bfp, 0);
    auto res_1win = wiz_win(bfp, 1);
    auto res_0lose = wiz_lose(bfp, 0);
    auto res_1lose = wiz_lose(bfp, 1);
    bool rm_bywin = res_0win.first || res_1win.first || cutting_w > 0;
    bool rm_bylose = res_0lose.first || res_1lose.first || cutting_l > 0;
    assert(0 <= res_0win.second && res_0win.second < 11);
    assert(0 <= res_1win.second && res_1win.second < 11);

    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_0win.first && res_1win.first) {
      if(res_0win.second <= res_1win.second) win_points[res_0win.second]++;
      else win_points[res_1win.second]++;
    } else if(res_0win.first) win_points[res_0win.second]++;
    else if(res_1win.first) win_points[res_1win.second]++;
    else win_points[0]++;
    if(res_0lose.first && res_1lose.first) {
      if(res_0lose.second >= res_1lose.second) lose_points[res_0lose.second]++;
      else lose_points[res_1lose.second]++;
    } else if(res_0lose.first) lose_points[res_0lose.second]++;
    else if(res_1lose.first) lose_points[res_1lose.second]++;
    else lose_points[0]++;

    std::string action = oph.get_action((unsigned char)key[0]);
    bool is_zero = char_to_action(action[0]) / 10 == 0;
    wf[n.depth] = {{res_0win.first, res_1win.first},
                   {res_0lose.first, res_1lose.first},
                   is_zero};
  }
  // to_self == false: 相手に使う分岐 (do_action 6)
  // to_self == true : 自分に使う分岐 (do_action 7)
  static int wiz_index(bool to_self, bool is_zero) {
    return (to_self == is_zero) ? 0 : 1;
  }
  void enter_wiz_branch(const node &n, bool to_self) {
    const int k = wiz_index(to_self, wf[n.depth].is_zero);
    cutting_w += wf[n.depth].win[k];
    cutting_l += wf[n.depth].lose[k];
  }
  void leave_wiz_branch(const node &n, bool to_self) {
    const int k = wiz_index(to_self, wf[n.depth].is_zero);
    cutting_w -= wf[n.depth].win[k];
    cutting_l -= wf[n.depth].lose[k];
  }
};

#endif
