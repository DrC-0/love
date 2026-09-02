#ifndef BF_POSITION_HPP
#include "bf_position.hpp"
#endif
bool cutting_w = false;
bool cutting_l = false;
int end_deck_n = 0;
void org_ds_put_hide_card(node &n);
void org_ds_draw_p1_init(node &n);
void org_ds_draw_p2_init(node &n);
void org_ds_draw(node &n);
void org_ds_play(node &n, int c);
void org_ds_wizard(node &n);
void org_ds_wizard_self(node &n);
void org_ds_soldior(node &n); // 兵士用の状態分岐関数を追加

void org_ds_put_hide_card(node &n) {
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(2, i, w);
    org_ds_draw_p1_init(n);
    n.undo_action(2, i, w);
  }
  return;
}

void org_ds_draw_p1_init(node &n) {
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(3, i, w);
    org_ds_draw_p2_init(n);
    n.undo_action(3, i, w);
  }
  return;
}

void org_ds_draw_p2_init(node &n) {
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n);
    n.undo_action(4, i, w);
  }
  return;
}

void org_ds_draw(node &n) {
  if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) {
    end_points++;
    return;
  }
  //必勝判定
  if(use_good_move) {
    if(n.open2 > 1 && n.barrier2 == false) {
      if(n.hand1[0] == 1 || n.hand1[1] == 1) {
        end_points++;
        return;
      }
    }
    if(n.open2 > 0 && n.barrier2 == false) {
      if(n.hand1[0] == 3 && n.hand1[1] > n.open2) {
        end_points++;
        return;
      }
      if(n.hand1[1] == 3 && n.hand1[0] > n.open2) {
        end_points++;
        return;
      }
    }
    if(n.open2 == 8 && n.barrier2 == false) {
      if(n.hand1[0] == 5 || n.hand1[1] == 5) {
        end_points++;
        return;
      }
    }
  }
  work_do_action w;
  //必敗判定
  if(nuse_bad_move) {
    if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) {
      end_points++;
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) {
      end_points++;
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      org_ds_play(n, card);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[0] == 8) {
      end_points++;
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card);
      n.undo_action(5, card, w);
      return;
    } else if(n.hand1[1] == 8) {
      end_points++;
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      org_ds_play(n, card);
      n.undo_action(5, card, w);
      return;
    }
  }
  //手札が同じカード2枚のとき
  if(use_same_move) {
    if(n.hand1[0] == n.hand1[1]) {
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      org_ds_play(n, card);
      n.undo_action(5, card, w);
      return;
    }
  }

  if(n.turn == 0) {
    p1_points++;
  } else {
    p2_points++;
  }
  int c1, c2;
  c1 = n.hand1[0];
  c2 = n.hand1[1];
  string key = n.org_his_p[n.turn].get_hash_value();
  bf_position bfp(n.open, key, false);
  auto res_0win = use_win(bfp, c1);
  auto res_1win = use_win(bfp, c2);
  auto res_0lose = use_lose(bfp, c1);
  auto res_1lose = use_lose(bfp, c2);
  bool rm_bywin = res_0win.first || res_1win.first || cutting_w;
  bool rm_bylose = res_0lose.first || res_1lose.first || cutting_l;
  bool con_cutting_w = cutting_w;
  bool con_cutting_l = cutting_l;
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
  // if(res_0win.first || res_1win.first) output_actions_history(key, false);

  // w.infset_it = table_infset.find(key);
  static auto dummy_it = table_infset.emplace("__dummy__", infset{}).first;
  w.infset_it = dummy_it;
  work_do_action w2 = w;
  if(res_0win.first || res_1win.first) cutting_w = true;
  if(res_0lose.first) cutting_l = true;
  n.do_action(5, 0, w);
  org_ds_play(n, c1);
  n.undo_action(5, c1, w);
  if(res_0lose.first && !con_cutting_l) cutting_l = false;
  if(res_1lose.first) cutting_l = true;
  n.do_action(5, 1, w2);
  org_ds_play(n, c2);
  n.undo_action(5, c2, w2);
  if(res_1lose.first && !con_cutting_l) cutting_l = false;
  if((res_0win.first || res_1win.first) && !con_cutting_w) cutting_w = false;
  return;
}

void org_ds_play(node &n, int c) {
  switch(c) {
  case 1:
    if(n.open2 > 1 && n.barrier2 == false) {
      end_points++;
      return;
    }
    if(n.barrier2 == false) {
      if(n.turn == 0) {
        p1_points++;
      } else {
        p2_points++;
      }
      // 兵士の推測行動用のInformation Setを作成
      string key = n.org_his_p[n.turn].get_hash_value();

      work_do_action ds_w0;
      static auto dummy_it = table_infset.emplace("__dummy__", infset{}).first;
      ds_w0.infset_it = dummy_it;

      bool candidate[8] = {false, false, false, false, false, false, false, false};
      candidate[n.hide - 1] = true;
      candidate[n.hand2 - 1] = true;
      bf_position bfp(n.open, key, false);
      for(int i = 2; i < 9; i++) {
        if(n.deck[i - 1] > 0) candidate[i - 1] = true;
        if(!candidate[i - 1]) continue;
        auto res_win = sol_win(bfp, i);
        bool rm_bywin = res_win.first || cutting_w;
        bool rm_bylose = cutting_l;
        bool con_cutting_w = cutting_w;
        assert(0 <= res_win.second && res_win.second < 11);
        decision_points[0]++;
        if(!rm_bywin) decision_points[1]++;
        if(!rm_bylose) decision_points[2]++;
        if(!rm_bywin && !rm_bylose) decision_points[3]++;
        if(res_win.first) win_points[res_win.second]++;
        else win_points[0]++;

        if(i == n.hand2) {
          // 推測が的中した場合は即座にゲーム終了
          end_points++;
        } else {
          // 不正解の場合は次のドローフェーズ(org_ds_soldior)へ移行
          if(res_win.first) cutting_w = true;
          work_do_action ds_w = ds_w0;
          n.do_action(8, i, ds_w);
          org_ds_soldior(n);
          n.undo_action(8, i, ds_w);
          if(res_win.first && !con_cutting_w) cutting_w = false;
        }
      }
      // 山札などの関係で選択肢が全くない場合
      if(!candidate[1] && !candidate[2] && !candidate[3] && !candidate[4] && !candidate[5] && !candidate[6] && !candidate[7]) {
        org_ds_soldior(n);
        // exit(1); // ここは本来ありえないはずなので、異常終了させる
      }
      return;
    }
    break;
  case 2:
    break;
  case 3:
    if(n.hand1[0] > n.hand2 && n.barrier2 == false) {
      end_points++;
      return;
    } else if(n.hand1[0] < n.hand2 && n.barrier2 == false) {
      end_points++;
      return;
    }
    break;
  case 4:
    break;
  case 5: {
    if(n.turn == 0) {
      p1_points++;
    } else {
      p2_points++;
    }
    string key = n.org_his_p[n.turn].get_hash_value();
    bf_position bfp(n.open, key, false);
    auto res_0win = wiz_win(bfp, 0);
    auto res_1win = wiz_win(bfp, 1);
    auto res_0lose = wiz_lose(bfp, 0);
    auto res_1lose = wiz_lose(bfp, 1);
    bool rm_bywin = res_0win.first || res_1win.first || cutting_w;
    bool rm_bylose = res_0lose.first || res_1lose.first || cutting_l;
    bool con_cutting_w = cutting_w;
    bool con_cutting_l = cutting_l;
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
    string action = oph.get_action((unsigned char)key[0]);

    bool is_zero = char_to_action(action[0]) / 10 == 0;

    work_do_action ds_w0;
    // ds_w0.infset_it = table_infset.find(key);
    static auto dummy_it = table_infset.emplace("__dummy__", infset{}).first;
    ds_w0.infset_it = dummy_it;
    if(n.dsum() <= end_deck_n) {
      end_points++;
      return;
    } else {
      if(n.barrier2 == false) {
        if(n.hand2 != 8) {
          rand_points++;
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            if(res_0win.first && !is_zero) cutting_w = true;
            if(res_1win.first && is_zero) cutting_w = true;
            if(res_0lose.first && !is_zero) cutting_l = true;
            if(res_1lose.first && is_zero) cutting_l = true;
            work_do_action ds_w2 = ds_w0;
            n.do_action(6, i, ds_w2);
            org_ds_wizard(n);
            n.undo_action(6, i, ds_w2);
            if(res_0win.first && !is_zero && !con_cutting_w) cutting_w = false;
            if(res_1win.first && is_zero && !con_cutting_w) cutting_w = false;
            if(res_0lose.first && !is_zero && !con_cutting_l) cutting_l = false;
            if(res_1lose.first && is_zero && !con_cutting_l) cutting_l = false;
          }
        } else {
          end_points++;
        }
      } else {
        // バリア展開時は org_his 構造に沿って (6, 0) を実行する形に修正
        if(res_0win.first && !is_zero) cutting_w = true;
        if(res_1win.first && is_zero) cutting_w = true;
        if(res_0lose.first && !is_zero) cutting_l = true;
        if(res_1lose.first && is_zero) cutting_l = true;
        work_do_action ds_ww = ds_w0;
        n.do_action(6, 0, ds_ww);
        org_ds_wizard(n);
        n.undo_action(6, 0, ds_ww);
        if(res_0win.first && !is_zero && !con_cutting_w) cutting_w = false;
        if(res_1win.first && is_zero && !con_cutting_w) cutting_w = false;
        if(res_0lose.first && !is_zero && !con_cutting_l) cutting_l = false;
        if(res_1lose.first && is_zero && !con_cutting_l) cutting_l = false;
      }
      if(n.hand1[0] != 8) {
        rand_points++;
        for(int i = 1; i < 9; i++) {
          if(n.deck[i - 1] == 0) {
            continue;
          }
          if(res_0win.first && is_zero) cutting_w = true;
          if(res_1win.first && !is_zero) cutting_w = true;
          if(res_0lose.first && is_zero) cutting_l = true;
          if(res_1lose.first && !is_zero) cutting_l = true;
          work_do_action ds_w3 = ds_w0;
          n.do_action(7, i, ds_w3);
          org_ds_wizard_self(n);
          n.undo_action(7, i, ds_w3);
          if(res_0win.first && is_zero && !con_cutting_w) cutting_w = false;
          if(res_1win.first && !is_zero && !con_cutting_w) cutting_w = false;
          if(res_0lose.first && is_zero && !con_cutting_l) cutting_l = false;
          if(res_1lose.first && !is_zero && !con_cutting_l) cutting_l = false;
        }
      } else {
        end_points++;
      }
      return;
    }
  } break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    end_points++;
    return;
  }
  if(n.dsum() <= end_deck_n) {
    end_points++;
    return;
  }
  // if(is_openhand_loveletter_his(n.open, n.org_his_p[0].get_hash_value(), n.org_his_p[1].get_hash_value(), false)){
  //   opengame++;
  // }

  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n);
    n.undo_action(4, i, w);
  }
  return;
}

// 兵士推測不正解後のドロー・ターン進行処理
void org_ds_soldior(node &n) {
  if(n.dsum() <= end_deck_n) {
    end_points++;
    return;
  }
  // if(is_openhand_loveletter_his(n.open, n.org_his_p[0].get_hash_value(), n.org_his_p[1].get_hash_value(), false)){
  //   opengame++;
  // }

  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n);
    n.undo_action(4, i, w);
  }
  return;
}

void org_ds_wizard(node &n) {
  if(n.dsum() <= end_deck_n) {
    end_points++;
    return;
  }
  // if(is_openhand_loveletter_his(n.open, n.org_his_p[0].get_hash_value(), n.org_his_p[1].get_hash_value(), false)){
  //   opengame++;
  // }

  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n);
    n.undo_action(4, i, w);
  }
  return;
}

void org_ds_wizard_self(node &n) {
  if(n.dsum() <= end_deck_n) {
    end_points++;
    return;
  }
  // if(is_openhand_loveletter_his(n.open, n.org_his_p[0].get_hash_value(), n.org_his_p[1].get_hash_value(), false)){
  //   opengame++;
  // }

  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    n.do_action(4, i, w);
    org_ds_draw(n);
    n.undo_action(4, i, w);
  }
  return;
}
