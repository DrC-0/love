#ifndef ORG_TREE_HPP
#define ORG_TREE_HPP

#include "loveletter.hpp"

// 打ち切り深さ: 山札の残り枚数がこの値以下になった時点で探索を終える。
// 解析を早く終わらせて異常検知や深さの調査に使う道具であり、ゲームのルールではない。
// cfr_org.cpp の main が argv[4] で上書きする。既定の 0 は現行と完全に同一の挙動。
inline int end_deck_n = 0;

template <class V>
void org_ds_put_hide_card(node &n, V &v);
template <class V>
void org_ds_draw_p1_init(node &n, V &v);
template <class V>
void org_ds_draw_p2_init(node &n, V &v);
template <class V>
void org_ds_draw(node &n, V &v);
template <class V>
void org_ds_play(node &n, int c, V &v);
template <class V>
void org_ds_wizard(node &n, V &v);
template <class V>
void org_ds_wizard_self(node &n, V &v);
template <class V>
void org_ds_soldior(node &n, V &v);

template <class V>
void org_ds_put_hide_card(node &n, V &v) {
  v.on_chance();
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
  v.on_chance();
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
  v.on_chance();
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
    v.on_terminal();
    return;
  }
  //必勝判定
  if(use_good_move) {
    if(n.open2 > 1 && n.barrier2 == false) {
      if(n.hand1[0] == 1 || n.hand1[1] == 1) {
        v.on_terminal();
        return;
      }
    }
    if(n.open2 > 0 && n.barrier2 == false) {
      if(n.hand1[0] == 3 && n.hand1[1] > n.open2) {
        v.on_terminal();
        return;
      }
      if(n.hand1[1] == 3 && n.hand1[0] > n.open2) {
        v.on_terminal();
        return;
      }
    }
    if(n.open2 == 8 && n.barrier2 == false) {
      if(n.hand1[0] == 5 || n.hand1[1] == 5) {
        v.on_terminal();
        return;
      }
    }
  }
  work_do_action w;
  //必敗判定
  if(nuse_bad_move) {
    if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) {
      v.on_terminal();
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) {
      v.on_terminal();
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[0] == 8) {
      v.on_terminal();
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card, v);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[1] == 8) {
      v.on_terminal();
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

  v.on_decision(n.turn);
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
      v.on_terminal();
      return;
    }
    if(n.barrier2 == false) {
      v.on_decision(n.turn);

      work_do_action ds_w0;
      ds_w0.infset_it = v.infset_for(n);

      bool candidate[8] = {false, false, false, false, false, false, false, false};
      candidate[n.hide - 1] = true;
      candidate[n.hand2 - 1] = true;
      v.enter_soldier(n);
      for(int i = 2; i < 9; i++) {
        if(n.deck[i - 1] > 0) candidate[i - 1] = true;
        if(!candidate[i - 1]) continue;
        v.on_guess(n, i);

        if(i == n.hand2) {
          // 推測が的中した場合は即座にゲーム終了
          v.on_terminal();
        } else {
          // 不正解の場合は次のドローフェーズ(org_ds_soldior)へ移行
          v.enter_guess(n);
          work_do_action ds_w = ds_w0;
          n.do_action(8, i, ds_w);
          org_ds_soldior(n, v);
          n.undo_action(8, i, ds_w);
          v.leave_guess(n);
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
      v.on_terminal();
      return;
    } else if(n.hand1[0] < n.hand2 && n.barrier2 == false) {
      v.on_terminal();
      return;
    }
    break;
  case 4:
    break;
  case 5: {
    v.on_decision(n.turn);
    v.enter_wizard(n);

    work_do_action ds_w0;
    ds_w0.infset_it = v.infset_for(n);

    if(n.dsum() <= end_deck_n) {
      v.on_terminal();
      return;
    } else {
      if(n.barrier2 == false) {
        if(n.hand2 != 8) {
          v.on_chance();
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            v.enter_wiz_branch(n, false);
            work_do_action ds_w2 = ds_w0;
            n.do_action(6, i, ds_w2);
            org_ds_wizard(n, v);
            n.undo_action(6, i, ds_w2);
            v.leave_wiz_branch(n, false);
          }
        } else {
          v.on_terminal();
        }
      } else {
        // バリア展開時は org_his 構造に沿って (6, 0) を実行する形に修正
        v.enter_wiz_branch(n, false);
        work_do_action ds_ww = ds_w0;
        n.do_action(6, 0, ds_ww);
        org_ds_wizard(n, v);
        n.undo_action(6, 0, ds_ww);
        v.leave_wiz_branch(n, false);
      }
      if(n.hand1[0] != 8) {
        v.on_chance();
        for(int i = 1; i < 9; i++) {
          if(n.deck[i - 1] == 0) {
            continue;
          }
          v.enter_wiz_branch(n, true);
          work_do_action ds_w3 = ds_w0;
          n.do_action(7, i, ds_w3);
          org_ds_wizard_self(n, v);
          n.undo_action(7, i, ds_w3);
          v.leave_wiz_branch(n, true);
        }
      } else {
        v.on_terminal();
      }
      return;
    }
  } break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    v.on_terminal();
    return;
  }
  if(n.dsum() <= end_deck_n) {
    v.on_terminal();
    return;
  }

  v.on_chance();
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
    v.on_terminal();
    return;
  }
  v.on_chance();
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
    v.on_terminal();
    return;
  }
  v.on_chance();
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
    v.on_terminal();
    return;
  }
  v.on_chance();
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
