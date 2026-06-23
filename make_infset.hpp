void org_ds_put_hide_card(node &n);
void org_ds_draw_p1_init(node &n);
void org_ds_draw_p2_init(node &n);
void org_ds_draw(node &n);
void org_ds_play(node &n, int c);
void org_ds_wizard(node &n);
void org_ds_wizard_self(node &n);
void org_ds_soldior(node &n); // 兵士用の状態分岐関数を追加

void org_ds_put_hide_card(node &n){
    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(2, i, w);
      org_ds_draw_p1_init(n);
      n.undo_action(2, i, w);
    }
    return;
}

void org_ds_draw_p1_init(node &n){
    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(3, i, w);
      org_ds_draw_p2_init(n);
      n.undo_action(3, i, w);
    }
    return;
}

void org_ds_draw_p2_init(node &n){
    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(4, i, w);
      org_ds_draw(n);
      n.undo_action(4, i, w);
    }
    return;
}

void org_ds_draw(node &n){
    if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) { end_points++;
      return;
    }
    //必勝判定
    if(use_good_move){
      if(n.open2 > 1 && n.barrier2 == false){
        if(n.hand1[0] == 1 || n.hand1[1] == 1){ end_points++;
          return;
        }
      }
      if(n.open2 > 0 && n.barrier2 == false){
        if(n.hand1[0] == 3 && n.hand1[1] > n.open2){ end_points++;
          return;
        }
        if(n.hand1[1] == 3 && n.hand1[0] > n.open2){ end_points++;
          return;
        }
      }
      if(n.open2 == 8 && n.barrier2 == false){
        if(n.hand1[0] == 5 || n.hand1[1] == 5){ end_points++;
          return;
        }
      }
    }
    work_do_action w;
    //必敗判定
    if(nuse_bad_move){
      if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false){ end_points++;
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        org_ds_play(n, card);
        n.undo_action(5, card, w);
        return;
      } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false){ end_points++;
        int card = n.hand1[0];
        n.do_action(5, 0, w);
        org_ds_play(n, card);
        n.undo_action(5, card, w);
        return;
      } else if(n.hand1[0] == 8) { end_points++;
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        org_ds_play(n, card);
        n.undo_action(5, card, w);
        return;
      } else if(n.hand1[1] == 8){ end_points++;
        int card = n.hand1[0];
        n.do_action(5, 0, w);
        org_ds_play(n, card);
        n.undo_action(5, card, w);
        return;
      }
    }
    //手札が同じカード2枚のとき
    if(use_same_move){
      if(n.hand1[0] == n.hand1[1]){
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        org_ds_play(n, card);
        n.undo_action(5, card, w);
        return;
      }
    }

    if(n.turn == 0){
      p1_points++;
    } else {
      p2_points++;
    }
    int c1, c2;
    // org_his_p を使用
    string key = n.org_his_p[n.turn].get_hash_value();
    infset &x = table_infset[key];

    if(all_elements_test) {
#ifdef ALL_ELEMENTS_TEST
      cout << "org_ds_draw history add" << endl;
      x.history.emplace_back(n.org_his.get_hash_value());
      x.history.shrink_to_fit();
#endif
    }
    if(x.get_pi_i() > 0 && abs(x.get_pi_i() - n.npi[n.turn][n.depth]) > pow(10.0, -10.0)) terminate();
    x.set_pi_i(n.npi[n.turn][n.depth]); assert(x.get_pi_i() != 0);
    x.depth = n.count_turn;
    x.play = true;
#ifdef CHECK_STR
    cout << "org_ds_draw check_str" << endl;
    if(n.hand1[0] == 6) x.general0 = true;
    if(n.hand1[1] == 6) x.general1 = true;
    if(n.hand1[0] == 7) x.minister0 = true;
    if(n.hand1[1] == 7) x.minister1 = true;
#endif
    w.infset_it = table_infset.find(key);
    work_do_action w2 = w;
    c1 = n.hand1[0]; c2 = n.hand1[1];
    n.do_action(5, 0, w);
    org_ds_play(n, c1);
    n.undo_action(5, c1, w);
    n.do_action(5, 1, w2);
    org_ds_play(n, c2);
    n.undo_action(5, c2, w2);
    return;
}

void org_ds_play(node &n, int c){
    switch(c) {
        case 1:
          if(n.open2 > 1 && n.barrier2 == false) { end_points++;
            return;
          }
          if(n.barrier2 == false){
            if(n.turn == 0){
              p1_points++;
            } else {
              p2_points++;
            }
            // 兵士の推測行動用のInformation Setを作成
            string key = n.org_his_p[n.turn].get_hash_value();
            infset &x = table_infset[key];
            if(all_elements_test) {
#ifdef ALL_ELEMENTS_TEST
              x.history.emplace_back(n.org_his.get_hash_value());
              x.history.shrink_to_fit();
#endif
            }
            if(x.get_pi_i() > 0 && abs(x.get_pi_i() - n.npi[n.turn][n.depth]) > pow(10.0, -10.0)) terminate();
            x.set_pi_i(n.npi[n.turn][n.depth]); assert(x.get_pi_i() != 0);
            x.depth = n.count_turn;
            x.soldior = true;

            work_do_action ds_w0;
            ds_w0.infset_it = table_infset.find(key);

            bool candidate[8] = {false, false, false, false, false, false, false, false};
            candidate[n.hide-1] = true;
            candidate[n.hand2-1] = true;
            for(int i = 2; i < 9; i++){
              if(n.deck[i-1] > 0) candidate[i-1] = true;
              if(!candidate[i-1]) continue;

              if(i == n.hand2){
                // 推測が的中した場合は即座にゲーム終了
                end_points++;
              } else {
                // 不正解の場合は次のドローフェーズ(org_ds_soldior)へ移行
                work_do_action ds_w = ds_w0;
                n.do_action(8, i, ds_w);
                org_ds_soldior(n);
                n.undo_action(8, i, ds_w);
              }
            }
            // 山札などの関係で選択肢が全くない場合
            if(!candidate[1] && !candidate[2] && !candidate[3] && !candidate[4] && !candidate[5] && !candidate[6] && !candidate[7]){
              // org_ds_soldior(n);
              exit(1); // ここは本来ありえないはずなので、異常終了させる
            }
            return;
          }
          break;
        case 2:
          break;
        case 3:
          if(n.hand1[0] > n.hand2 && n.barrier2 == false) { end_points++;
            return;
          } else if (n.hand1[0] < n.hand2 && n.barrier2 == false){ end_points++;
            return;
          }
          break;
        case 4:
          break;
        case 5: {
          if(n.turn == 0){
            p1_points++;
          } else {
            p2_points++;
          }
          // org_his_p を使用
          string key = n.org_his_p[n.turn].get_hash_value();
          infset &x = table_infset[key];
          if(all_elements_test) {
#ifdef ALL_ELEMENTS_TEST
            x.history.emplace_back(n.org_his.get_hash_value());
            x.history.shrink_to_fit();
#endif
          }
          if(x.get_pi_i() > 0 && abs(x.get_pi_i() - n.npi[n.turn][n.depth]) > pow(10.0, -10.0)) terminate();
          x.set_pi_i(n.npi[n.turn][n.depth]); assert(x.get_pi_i() != 0);
          x.depth = n.count_turn;
          x.wizard = true;
#ifdef CHECK_STR
    if(n.hand1[0] == 6) x.wizard_with_general = true;
    if(n.hand1[0] == 8) x.wizard_with_princess = true;
#endif
          work_do_action ds_w0;
          ds_w0.infset_it = table_infset.find(key);
          if(n.dsum() == 0){ end_points++;
            return;
          } else {
            if(n.barrier2 == false){
              if(n.hand2 != 8){
                rand_points++;
                for(int i = 1; i < 9; i++){
                  if(n.deck[i-1] == 0) { continue; }
                  work_do_action ds_w2 = ds_w0;
                  n.do_action(6, i, ds_w2);
                  org_ds_wizard(n);
                  n.undo_action(6, i, ds_w2);
                }
              } else {
                end_points++;
              }
            } else {
              // バリア展開時は org_his 構造に沿って (6, 0) を実行する形に修正
              work_do_action ds_ww = ds_w0;
              n.do_action(6, 0, ds_ww);
              org_ds_wizard(n);
              n.undo_action(6, 0, ds_ww);
            }
            if(n.hand1[0] != 8){
              rand_points++;
              for(int i = 1; i < 9; i++){
                if(n.deck[i-1] == 0) { continue; }
                work_do_action ds_w3 = ds_w0;
                n.do_action(7, i, ds_w3);
                org_ds_wizard_self(n);
                n.undo_action(7, i, ds_w3);
              }
            } else {
              end_points++;
            }
            return;
          }
        }
          break;
        case 6:
          break;
        case 7:
          break;
        case 8:
          end_points++;
          return;
    }
    if(n.dsum() == 0){ end_points++;
      return;
    }

    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(4, i, w);
      org_ds_draw(n);
      n.undo_action(4, i, w);
    }
    return;
}

// 兵士推測不正解後のドロー・ターン進行処理
void org_ds_soldior(node &n){
    if(n.dsum() == 0){ end_points++;
      return;
    }

    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(4, i, w);
      org_ds_draw(n);
      n.undo_action(4, i, w);
    }
    return;
}

void org_ds_wizard(node &n){
    if(n.dsum() == 0){ end_points++;
      return;
    }

    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(4, i, w);
      org_ds_draw(n);
      n.undo_action(4, i, w);
    }
    return;
}

void org_ds_wizard_self(node &n){
    if(n.dsum() == 0){ end_points++;
      return;
    }

    rand_points++;
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(4, i, w);
      org_ds_draw(n);
      n.undo_action(4, i, w);
    }
    return;
}