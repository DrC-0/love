# make_infset.hpp を展開器と判定器に分離する

## 1. 目的と射程

`make_infset.hpp` は org のゲーム木の展開と、必勝・必敗判定の呼び出し・統計の集計が
同一の DFS に融合している。これを次の 2 ファイルに分離する。

- **`org_tree.hpp`（展開器）**: ゲーム木を DFS で展開する。何を測るかは知らない。
- **`visit_winlose.hpp`（判定器）**: 各ノードで必勝・必敗判定を呼び、統計を集計する。

接続は **関数テンプレート + policy 型**。`org_ds_*` を `template <class V>` にし、
判定器のインスタンス `V &v` を引数で受け取る。仮想関数・`std::function`・マクロは使わない
（理由は `docs/adr/0002-template-based-walker-visitor.md`）。

用語は `CONTEXT.md` に従う。「意思決定点」「自然手番」「展開器」「判定器」
「削減対象マーカー」。

## 2. 触るファイル / 触らないファイル

### 触る
| ファイル | 変更 |
|---|---|
| `org_tree.hpp` | **新規** |
| `visit_winlose.hpp` | **新規** |
| `cfr_org.cpp` | include 差し替え、`main` の argv[4]、`cfr_org()` の呼び出し |
| `Makefile` | `cfrorg` / `cfrorgd` の依存行 |

### 絶対に触らない
`rnd_make_infset.hpp` / `infset_dfs.hpp` / `infset_dfs_rnd.hpp` / `all_elements.hpp` /
`all_elements_rnd.hpp` / `cfr.hpp` / `newcfr.hpp` / `cfr_exp_reward.hpp` /
`loveletter.cpp` / `loveletter.hpp` / `bf_position.hpp` / その他すべての `.cpp`。

`make_infset.hpp` は**今回は削除も変更もしない**。分離後にビット一致が取れてから
別作業で削除する。`cfr_org.cpp` が include しなくなるので、どこからも参照されない
状態で残る。

**この変更は org 側だけを触る。** rnd 側を触らなくてよい理由: 今回の目的は
「org の展開部分と判定の融合を解く」ことであり、`rnd_make_infset.hpp` にはそもそも
判定が入っていない（`bf_position` も `is_win` も出てこず、`table_infset` に情報集合を
積むだけ）。rnd 側の判定は `infset_iswin.cpp` が第2フェーズとして別に持っており、
既に分離済みである。org と rnd の展開器を1本に畳むのは将来の別作業。

**行動履歴には一切手を入れない。** 追記も、末尾1文字の payload 付き置換もしない。
`node::do_action` / `node::undo_action` をそのまま呼ぶだけで、履歴の生成規則は不変。

**`node::do_action` の `npi`（到達確率）まわりは触らない。**

## 3. `org_tree.hpp`（新規・全文）

```cpp
#ifndef ORG_TREE_HPP
#define ORG_TREE_HPP

#include "loveletter.hpp"

// 打ち切り深さ: 山札の残り枚数がこの値以下になった時点で探索を終える。
// 解析を早く終わらせて異常検知や深さの調査に使う道具であり、ゲームのルールではない。
// cfr_org.cpp の main が argv[4] で上書きする。既定の 0 は現行と完全に同一の挙動。
inline int end_deck_n = 0;

template <class V> void org_ds_put_hide_card(node &n, V &v);
template <class V> void org_ds_draw_p1_init(node &n, V &v);
template <class V> void org_ds_draw_p2_init(node &n, V &v);
template <class V> void org_ds_draw(node &n, V &v);
template <class V> void org_ds_play(node &n, int c, V &v);
template <class V> void org_ds_wizard(node &n, V &v);
template <class V> void org_ds_wizard_self(node &n, V &v);
template <class V> void org_ds_soldior(node &n, V &v);

template <class V>
void org_ds_put_hide_card(node &n, V &v) {
  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(2, i, w);
    org_ds_draw_p1_init(n, v);
    n.undo_action(2, i, w);
  }
  return;
}

template <class V>
void org_ds_draw_p1_init(node &n, V &v) {
  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(3, i, w);
    org_ds_draw_p2_init(n, v);
    n.undo_action(3, i, w);
  }
  return;
}

template <class V>
void org_ds_draw_p2_init(node &n, V &v) {
  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n, v);
    n.undo_action(4, i, w);
  }
  return;
}

template <class V>
void org_ds_draw(node &n, V &v) {
  if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) {
    v.on_terminal_points();
    return;
  }
  //必勝判定
  if(use_good_move) {
    if(n.open2 > 1 && n.barrier2 == false) {
      if(n.hand1[0] == 1 || n.hand1[1] == 1) {
        v.on_terminal_points();
        return;
      }
    }
    if(n.open2 > 0 && n.barrier2 == false) {
      if(n.hand1[0] == 3 && n.hand1[1] > n.open2) {
        v.on_terminal_points();
        return;
      }
      if(n.hand1[1] == 3 && n.hand1[0] > n.open2) {
        v.on_terminal_points();
        return;
      }
    }
    if(n.open2 == 8 && n.barrier2 == false) {
      if(n.hand1[0] == 5 || n.hand1[1] == 5) {
        v.on_terminal_points();
        return;
      }
    }
  }
  work_do_action w;
  //必敗判定
  if(nuse_bad_move) {
    if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) {
      v.on_terminal_points();
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) {
      v.on_terminal_points();
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[0] == 8) {
      v.on_terminal_points();
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[1] == 8) {
      v.on_terminal_points();
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    }
  }
  //手札が同じカード2枚のとき
  if(use_same_move) {
    if(n.hand1[0] == n.hand1[1]) {
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    }
  }

  v.on_decision_points(n.turn);
  int c1, c2;
  c1 = n.hand1[0];
  c2 = n.hand1[1];

  v.enter_play(n, c1, c2);
  w.infset_it = v.infset_for(n);
  work_do_action w2 = w;
  n.do_action(5, 0, w);
  org_ds_play(n, c1, v);
  n.undo_action(5, c1, w);
  v.mid_play(n);
  n.do_action(5, 1, w2);
  org_ds_play(n, c2, v);
  n.undo_action(5, c2, w2);
  v.leave_play(n);
  return;
}

template <class V>
void org_ds_play(node &n, int c, V &v) {
  switch(c) {
  case 1:
    if(n.open2 > 1 && n.barrier2 == false) {
      v.on_terminal_points();
      return;
    }
    if(n.barrier2 == false) {
      v.on_decision_points(n.turn);

      work_do_action ds_w0;
      ds_w0.infset_it = v.infset_for(n);

      bool candidate[8] = {false, false, false, false, false, false, false, false};
      candidate[n.hide - 1] = true;
      candidate[n.hand2 - 1] = true;
      v.enter_soldier(n);
      for(int i = 2; i < 9; i++) {
        if(n.deck[i - 1] > 0) candidate[i - 1] = true;
        if(!candidate[i - 1]) continue;
        v.on_soldier_guess(n, i);

        if(i == n.hand2) {
          // 推測が的中した場合は即座にゲーム終了
          v.on_terminal_points();
        } else {
          // 不正解の場合は次のドローフェーズ(org_ds_soldior)へ移行
          v.enter_soldier_guess(n);
          work_do_action ds_w = ds_w0;
          n.do_action(8, i, ds_w);
          org_ds_soldior(n, v);
          n.undo_action(8, i, ds_w);
          v.leave_soldier_guess(n);
        }
      }
      // 山札などの関係で選択肢が全くない場合
      if(!candidate[1] && !candidate[2] && !candidate[3] && !candidate[4] && !candidate[5] && !candidate[6] && !candidate[7]) {
        org_ds_soldior(n, v);
      }
      return;
    }
    break;
  case 2:
    break;
  case 3:
    if(n.hand1[0] > n.hand2 && n.barrier2 == false) {
      v.on_terminal_points();
      return;
    } else if(n.hand1[0] < n.hand2 && n.barrier2 == false) {
      v.on_terminal_points();
      return;
    }
    break;
  case 4:
    break;
  case 5: {
    v.on_decision_points(n.turn);
    v.enter_wizard(n);

    work_do_action ds_w0;
    ds_w0.infset_it = v.infset_for(n);

    if(n.dsum() <= end_deck_n) {
      v.on_terminal_points();
      return;
    } else {
      if(n.barrier2 == false) {
        if(n.hand2 != 8) {
          v.on_chance_points();
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            v.enter_wizard_branch(n, false);
            work_do_action ds_w2 = ds_w0;
            n.do_action(6, i, ds_w2);
            org_ds_wizard(n, v);
            n.undo_action(6, i, ds_w2);
            v.leave_wizard_branch(n, false);
          }
        } else {
          v.on_terminal_points();
        }
      } else {
        // バリア展開時は org_his 構造に沿って (6, 0) を実行する形に修正
        v.enter_wizard_branch(n, false);
        work_do_action ds_ww = ds_w0;
        n.do_action(6, 0, ds_ww);
        org_ds_wizard(n, v);
        n.undo_action(6, 0, ds_ww);
        v.leave_wizard_branch(n, false);
      }
      if(n.hand1[0] != 8) {
        v.on_chance_points();
        for(int i = 1; i < 9; i++) {
          if(n.deck[i - 1] == 0) {
            continue;
          }
          v.enter_wizard_branch(n, true);
          work_do_action ds_w3 = ds_w0;
          n.do_action(7, i, ds_w3);
          org_ds_wizard_self(n, v);
          n.undo_action(7, i, ds_w3);
          v.leave_wizard_branch(n, true);
        }
      } else {
        v.on_terminal_points();
      }
      return;
    }
  } break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    v.on_terminal_points();
    return;
  }
  if(n.dsum() <= end_deck_n) {
    v.on_terminal_points();
    return;
  }

  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n, v);
    n.undo_action(4, i, w);
  }
  return;
}

// 兵士推測不正解後のドロー・ターン進行処理
template <class V>
void org_ds_soldior(node &n, V &v) {
  if(n.dsum() <= end_deck_n) {
    v.on_terminal_points();
    return;
  }
  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n, v);
    n.undo_action(4, i, w);
  }
  return;
}

template <class V>
void org_ds_wizard(node &n, V &v) {
  if(n.dsum() <= end_deck_n) {
    v.on_terminal_points();
    return;
  }
  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n, v);
    n.undo_action(4, i, w);
  }
  return;
}

template <class V>
void org_ds_wizard_self(node &n, V &v) {
  if(n.dsum() <= end_deck_n) {
    v.on_terminal_points();
    return;
  }
  v.on_chance_points();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n, v);
    n.undo_action(4, i, w);
  }
  return;
}

#endif
```

### 元コードから削除した要素

- `#include "bf_position.hpp"`（1〜3行目）→ 展開器は不要になる
- `bool cutting_w` / `bool cutting_l`（4〜5行目）→ 判定器へ
- `int end_deck_n = 0;`（6行目）→ `inline int` にして残す
- `p1_points++` / `p2_points++` / `rand_points++` / `end_points++` → フック呼び出しに置換
- `string key = ...` / `bf_position bfp(...)` / `use_win` / `use_lose` / `sol_win` /
  `wiz_win` / `wiz_lose` / `oph.get_action` / `is_zero` / `assert` / `decision_points` /
  `win_points` / `lose_points` / `static auto dummy_it` → すべて判定器へ
- `// if(is_openhand_loveletter_his(...))` のコメントブロック（385-387, 408-410,
  430-432, 452-454行）→ **削除する**。展開器は `bf_position.hpp` を include しないため、
  復活させても動かないコメントを残さない。

## 4. `visit_winlose.hpp`（新規・全文）

```cpp
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
    bool w_inc;   // enter_play で cutting_w に足した分
    bool l0;      // 分岐0で cutting_l に足す分 (res_0lose.first)
    bool l1;      // 分岐1で cutting_l に足す分 (res_1lose.first)
  };
  play_frame pf[MAX_DEPTH]{};

  // org_ds_play case 1（兵士）用
  bf_position soldier_bfp[MAX_DEPTH]{};
  bool soldier_inc[MAX_DEPTH]{};   // 直前の on_soldier_guess が返した res_win.first

  // org_ds_play case 5（魔術師）用
  struct wizard_frame {
    bool win[2];    // {res_0win.first, res_1win.first}
    bool lose[2];   // {res_0lose.first, res_1lose.first}
    bool is_zero;
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
    soldier_bfp[n.depth] = bf_position(n.open, key, false);
  }
  void on_soldier_guess(const node &n, int i) {
    auto res_win = sol_win(soldier_bfp[n.depth], i);
    bool rm_bywin = res_win.first || cutting_w > 0;
    bool rm_bylose = cutting_l > 0;
    assert(0 <= res_win.second && res_win.second < 11);
    decision_points[0]++;
    if(!rm_bywin) decision_points[1]++;
    if(!rm_bylose) decision_points[2]++;
    if(!rm_bywin && !rm_bylose) decision_points[3]++;
    if(res_win.first) win_points[res_win.second]++;
    else win_points[0]++;
    soldier_inc[n.depth] = res_win.first;
  }
  void enter_soldier_guess(const node &n) {
    cutting_w += soldier_inc[n.depth];
  }
  void leave_soldier_guess(const node &n) {
    cutting_w -= soldier_inc[n.depth];
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
  static int wizard_index(bool to_self, bool is_zero) {
    return (to_self == is_zero) ? 0 : 1;
  }
  void enter_wizard_branch(const node &n, bool to_self) {
    const int k = wizard_index(to_self, wf[n.depth].is_zero);
    cutting_w += wf[n.depth].win[k];
    cutting_l += wf[n.depth].lose[k];
  }
  void leave_wizard_branch(const node &n, bool to_self) {
    const int k = wizard_index(to_self, wf[n.depth].is_zero);
    cutting_w -= wf[n.depth].win[k];
    cutting_l -= wf[n.depth].lose[k];
  }
};

#endif
```

### `wizard_index` の根拠（間違えると魔術師ノードの統計が壊れる）

元コードの相手側分岐（`make_infset.hpp:317-320`, `335-338`）:

```
if(res_0win.first  && !is_zero) cutting_w = true;
if(res_1win.first  &&  is_zero) cutting_w = true;
```

元コードの自分側分岐（`make_infset.hpp:354-357`）:

```
if(res_0win.first  &&  is_zero) cutting_w = true;
if(res_1win.first  && !is_zero) cutting_w = true;
```

つまり使われる添字は次の表のとおりで、`(to_self == is_zero) ? 0 : 1` と一致する。

| to_self | is_zero | 使う添字 |
|---|---|---|
| false（相手） | false | 0 |
| false（相手） | true  | 1 |
| true（自分）  | true  | 0 |
| true（自分）  | false | 1 |

## 5. カウンタ化の規約（`bool` → `int`）

元コードは `con_cutting_w` / `con_cutting_l` にブール値を退避し、
「祖先が立てていなければ戻す」を行っている。これはカウンタの `++` / `--` と厳密に等価:
祖先が 1 → 自分が 2 → 自分が抜けて 1、で祖先の分は残る。

守るべき規約:

1. `rm_bywin` / `rm_bylose` は **カウンタを増やす前**の値で判定する。
   `cutting_w > 0` と書き、`cutting_w` をそのまま真偽値に使わない。
2. `cutting_w` は **意思決定点単位**。`enter_play` で `+=`、`leave_play` で `-=`。
   分岐0と分岐1の両方をまたぐ。
3. `cutting_l` は **分岐単位**。分岐0は `res_0lose.first`、分岐1は `res_1lose.first`。
   `enter_play` で `+= l0`、`mid_play` で `-= l0; += l1`、`leave_play` で `-= l1`。
4. 兵士（case 1）の `cutting_w` は **推測カード i ごと**。`enter_soldier_guess` / `leave_soldier_guess`
   が再帰を挟む。`i == n.hand2` のときは再帰しないので `enter_soldier_guess` / `leave_soldier_guess` を
   呼ばない。ただし `on_soldier_guess`（統計）は候補 i すべてで呼ぶ。
5. 魔術師（case 5）の `cutting_w` / `cutting_l` は **ループの各反復ごと**。
   `enter_wizard_branch` / `leave_wizard_branch` が再帰を挟む。
   バリア展開時（`n.barrier2 == true`）の単発 `do_action(6, 0, ...)` も同じ扱い。
6. `enter_wizard` と `enter_soldier` は **カウンタを触らない**。
   `enter_wizard` の直後に `n.dsum() <= end_deck_n` で早期 return する経路があるため、
   ここでカウンタを増やすと `leave` が呼ばれず不均衡になる。
7. `n.open` を `bf_position` に渡すフック（`enter_play` / `enter_soldier` /
   `enter_wizard`）だけが `node &n` を **非 const 参照**で受け取る。
   `bf_position(int open[3], std::string, bool)` の第1引数が非 const 配列なので、
   これらを `const node &` にすると `bf_position(n.open, ...)` がコンパイルできない。
   `n.depth` しか使わない残りのフック（`mid_play` / `leave_play` / `on_soldier_guess` /
   `enter_soldier_guess` / `leave_soldier_guess` / `enter_wizard_branch` / `leave_wizard_branch`）は
   `const node &n` を受け取る。展開器は `node &n` を渡すので、どちらにも束縛できる。
   シグネチャを一律に非 const で揃えると cppcheck の `constParameter` が 7 件出て、
   ファイル単位の抑制が必要になる。抑制は将来の本物の指摘まで隠すので採らない。

### `node::depth` を添字に使ってよい根拠

`node::do_action` は switch の外で無条件に `depth++`（`loveletter.cpp:1021`）、
`node::undo_action` も無条件に `depth--`（`loveletter.cpp:1284`）する。
したがって do/undo が対で呼ばれる限り、同時にアクティブなフレームで `depth` は一意。

各フックの呼び出し位置と深さ:

- `enter_play` は `do_action(5, ...)` の前、`leave_play` は `undo_action` の後。同じ深さ。
- `enter_soldier` / `on_soldier_guess` / `enter_soldier_guess` は `do_action(8, ...)` の前、
  `leave_soldier_guess` は `undo_action` の後。同じ深さ。
- `enter_wizard` / `enter_wizard_branch` は `do_action(6 or 7, ...)` の前、
  `leave_wizard_branch` は `undo_action` の後。同じ深さ。
- `org_ds_draw` の `enter_play`（深さ d）から `do_action(5, ...)` を経て
  `org_ds_play` に入るので、`enter_wizard` / `enter_soldier` は深さ d+1 で動く。衝突しない。

同じ深さのスロットを兵士のループが反復ごとに再利用するが、
各反復は「設定 → 再帰 → 復元」で閉じているため重ならない。

## 6. `cfr_org.cpp` の変更

現行 `cfr_org.cpp:37`:
```cpp
#include "make_infset.hpp"
```
を次に置き換える（**`static Org_Perfect_Hash oph;`（35行目）より後**でなければならない。
`visit_winlose.hpp` が `oph` を参照するため）:
```cpp
#include "org_tree.hpp"
#include "visit_winlose.hpp"
```

`cfr_org()` の先頭（`cfr_org.cpp:41-43`）:
```cpp
void cfr_org(int open[3]) {
  node n_ds(open);
  rand_points++;
```
を:
```cpp
void cfr_org(int open[3]) {
  node n_ds(open);
  winlose_visitor v;
  v.on_chance_points();
```

同関数のループ本体（`cfr_org.cpp:49-51`）:
```cpp
    n_ds.do_action(1, i, ds_w);
    org_ds_put_hide_card(n_ds);
    n_ds.undo_action(1, i, ds_w);
```
を:
```cpp
    n_ds.do_action(1, i, ds_w);
    org_ds_put_hide_card(n_ds, v);
    n_ds.undo_action(1, i, ds_w);
```

`main`（`cfr_org.cpp:70-`）:
```cpp
int main(int, char *argv[]) {
  int a, b, c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);
```
を:
```cpp
int main(int argc, char *argv[]) {
  int a, b, c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);
  if(argc > 4) end_deck_n = atoi(argv[4]);
```

`cout << "open : " ...` の直後に打ち切り深さも出す:
```cpp
  cout << "end_deck_n : " << end_deck_n << endl;
```

グローバル変数の定義（`cfr_org.cpp:22-34`）は**そのまま**。移動も削除もしない。

## 7. `Makefile` の変更

現行:
```make
cfrorg: cfr_org.cpp make_infset.hpp bf_position.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) bf_position.hpp cfr_org.cpp -o $@

cfrorgd: cfr_org.cpp make_infset.hpp bf_position.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEBUG_DEFS) -DCFR $(COMMON_SRCS) bf_position.hpp cfr_org.cpp -o cfrorg
```
の依存部分 `make_infset.hpp` を `org_tree.hpp visit_winlose.hpp` に置換する。
**コマンド行（`g++ ...`）は変更しない。**

```make
cfrorg: cfr_org.cpp org_tree.hpp visit_winlose.hpp bf_position.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEFS) -DCFR $(COMMON_SRCS) bf_position.hpp cfr_org.cpp -o $@

cfrorgd: cfr_org.cpp org_tree.hpp visit_winlose.hpp bf_position.hpp $(COMMON_SRCS) $(COMMON_HDRS)
	g++ -std=c++20 $(COMMON_WARN) -O2 $(COMMON_DEBUG_DEFS) -DCFR $(COMMON_SRCS) bf_position.hpp cfr_org.cpp -o cfrorg
```

## 8. 移動対応表

| `make_infset.hpp` の行 | 内容 | 移動先 |
|---|---|---|
| 1-3 | `#include "bf_position.hpp"` | `visit_winlose.hpp` |
| 4-5 | `cutting_w` / `cutting_l` | `visit_winlose.hpp`（int 化） |
| 6 | `end_deck_n` | `org_tree.hpp`（`inline int`） |
| 17, 31, 45, 312, 349, 389, 412, 434, 456 | `rand_points++` | `v.on_chance_points()` |
| 60, 67, 73, 77, 83, 92, 99, 106, 113, 194, 231, 254, 257, 307, 331, 368, 378, 382, 405, 427, 449 | `end_points++` | `v.on_terminal_points()` |
| 133-135, 199-201, 265-267 | `p1_points++` / `p2_points++` | `v.on_decision_points(n.turn)` |
| 140-176 | org_ds_draw の判定・統計・cutting 加算 | `v.enter_play(n, c1, c2)` |
| 171-173 | `dummy_it` | `v.infset_for(n)` |
| 180-181 | 分岐0→1 の cutting_l 付け替え | `v.mid_play(n)` |
| 185-186 | cutting_l / cutting_w の復元 | `v.leave_play(n)` |
| 204, 213 | 兵士の key と bfp 構築 | `v.enter_soldier(n)` |
| 217-227 | 兵士の sol_win と統計 | `v.on_soldier_guess(n, i)` |
| 234 | 兵士の cutting_w 加算 | `v.enter_soldier_guess(n)` |
| 239 | 兵士の cutting_w 復元 | `v.leave_soldier_guess(n)` |
| 269-305 | 魔術師の判定・統計・is_zero | `v.enter_wizard(n)` |
| 317-320, 335-338, 354-357 | 魔術師の cutting 加算 | `v.enter_wizard_branch(n, to_self)` |
| 325-328, 343-346, 362-365 | 魔術師の cutting 復元 | `v.leave_wizard_branch(n, to_self)` |
| 385-387, 408-410, 430-432, 452-454 | `is_openhand_loveletter_his` のコメント | 削除 |

## 9. 検証手順

### 9.1 ビルド

```
make cfr cfr0 cfrorg brrnd brorg win
```

6 ターゲットすべてが警告ゼロで通ること。`make all` は使わない
（`watch` が既存のリンクエラーで止まるため）。

### 9.2 基準値の採取（実装前に master 側で1回）

```
git stash            # または別ワークツリーで master をチェックアウト
# make_infset.hpp:6 の end_deck_n を 6 に書き換えてビルド
make cfrorg && ./cfrorg 5 5 7 > /tmp/base_edn6.txt
# 同様に 5 でも採取
```

実装後は argv で渡せるので書き換え不要。

### 9.3 ビット一致の確認

部分ゲームは **557**。打ち切り深さのラダーで上から順に確認する。

| `end_deck_n` | 期待 `decision_points[0]` | おおよその実行時間 |
|---|---|---|
| 6 | 1981047 | 約 5.5 秒 |
| 5 | 16082178 | 約 31 秒 |
| 4 | （実測して記録） | 約 4 分 |
| 0（フル） | 9436366423 | 約 6.4 時間 |

```
./cfrorg 5 5 7 6 > /tmp/new_edn6.txt
diff /tmp/base_edn6.txt /tmp/new_edn6.txt
```

`end_deck_n :` の出力行は新規追加なので diff から除外してよい。それ以外の
**全カウンタが完全一致**すること:
`p1_points`, `p2_points`, `rand_points`, `end_points`,
`win_points[0..10]`, `lose_points[0..10]`, `decision_points[0..3]`。

`end_deck_n = 0` のフル実行は最後に 1 回だけ。

### 9.4 既定挙動の同一性

`./cfrorg 5 5 7`（第4引数なし）が `./cfrorg 5 5 7 0` と同じ出力になること。

### 9.5 実行時間

`/usr/bin/time -v ./cfrorg 5 5 7 5` で経過時間とピーク RSS を記録して報告する。
**合格条件には含めない。** 現行はピーク RSS 3.6 MB、判定が実行時間の 66%。
速くなるなら歓迎、遅くなったら数値を添えて報告する。

### 9.6 整形と静的解析

```
git add -p && git clang-format     # clang-format 14
make cppcheck
```

新規ファイル 2 本は全体が新規行なので全体が整形対象。
cppcheck の新規指摘を残さない。抑制が必要なら理由を添えて
`.cppcheck-suppressions` か該当行直前の `// cppcheck-suppress <id>` に書く。

## 10. やらないこと

- `make_infset.hpp` の削除・変更
- rnd 側のファイルの変更
- `node::do_action` / `node::undo_action` / `npi` の変更
- 行動履歴の生成規則の変更
- フックの粒度の正規化（意思決定点への抽象化）は将来の別作業
- `bf_position` の再構築コストの最適化は将来の別作業

## 11. 事後の追記

計画のとおり実装・検収したあと、以下を追加で行った。この節より上の本文は
ゲートを通した時点の内容だが、コード断片のフック名だけは下の変更を反映してある。

1. **`.cppcheck-suppressions` への抑制を撤回した。** 5 章のルール7を
   「すべてのフックが非 const」から「`n.open` を使う 3 つだけが非 const」に改めた。
   一律に非 const で揃えると `constParameter` が 7 件出てファイル単位の抑制が必要になり、
   それは将来の本物の指摘まで隠すため。
2. **フック名を変更した。** 自然手番・終端・意思決定点のフックに `points` を付け、
   兵士・魔術師のフックに対象を明示した。
   `on_chance` → `on_chance_points`、`on_terminal` → `on_terminal_points`、
   `on_decision` → `on_decision_points`、`on_guess` → `on_soldier_guess`、
   `enter_guess` → `enter_soldier_guess`、`leave_guess` → `leave_soldier_guess`、
   `enter_wiz_branch` → `enter_wizard_branch`、`leave_wiz_branch` → `leave_wizard_branch`、
   `wiz_index` → `wizard_index`。`play` 系列は変更なし。
3. **`make_infset.hpp` を削除した。** 2 章と 10 章では「今回は削除しない」としていたが、
   これは分離のビット一致が取れるまで参照実装を残すための措置だった。
   部分ゲーム 557 で打ち切り深さ 6 / 5 / 4 に加えてフル探索（打ち切りなし）でも
   全カウンタが一致したため、役目を終えたとして削除した。
4. **兵士のフックを魔術師と同じ形に揃えた。** `enter_soldier`（ループ前に 1 回、
   `bf_position` を作るだけ）と `on_soldier_guess`（候補ごとに判定）を 1 つに合流し、
   `enter_soldier(node &n, int i)` にした。`*_soldier_guess` は `*_soldier_branch` に
   改名し、`enter_wizard` / `enter_wizard_branch` / `leave_wizard_branch` と対称にした。
   合流により `bf_position` が宣言カードの候補ごとに作り直しになるため、
   部分ゲーム 557 の打ち切り深さ 5 で実行時間が 26.78 秒から 29.37 秒へ約 9.7% 増える。
   読みやすさを優先した意図的な選択。
5. **判定器の内部で `bf_position` の作り直しを省いた。** 4 で入った約 9.7% の増加を
   回収するため、`enter_soldier` が深さごとに履歴キーと `bf_position` を覚え、
   キーが前回と同じなら作り直さないようにした。`bf_position` は
   （部分ゲームの3枚, 履歴キー, org/rnd）だけで決まり、前後2つは実行を通して
   不変なので、キーが同じなら同じ盤面になる。再帰は深さ n.depth + 1 以降の
   スロットを使うため、宣言カードの候補をまたいでスロットが壊れることはない。
   フックの形は 4 のまま。部分ゲーム 557 の打ち切り深さ 6 で 7 回ずつ測り、
   最小値で 4.88 秒から 4.66 秒に戻り、合流前の 4.68 秒と同等になった。
