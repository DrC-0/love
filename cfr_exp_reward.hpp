double ut_put_hide_card(node &n);
double ut_draw_p1_init(node &n);
double ut_draw_p2_init(node &n);
double ut_draw(node &n);
double ut_play(node &n, int c);
double ut_wizard(node &n);
double ut_wizard_self(node &n);

double ut_put_hide_card(node &n){
    double reward = 0.0;
    double p1 = n.dsum_rec();

    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      double p2 = n.deck[i-1];
      n.do_action(2, i, w);
      reward += ut_draw_p1_init(n) * p1 * p2;
      n.undo_action(2, i, w);
    }

    return reward;
}

double ut_draw_p1_init(node &n){ 
    double reward = 0.0;
    double p1 = n.dsum_rec();

    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      double p2 = n.deck[i-1];
      n.do_action(3, i, w);
      reward += ut_draw_p2_init(n)  * p1 * p2;
      n.undo_action(3, i, w);
    }

    return reward;
}

double ut_draw_p2_init(node &n){ 
    double reward = 0.0;
    double p1 = n.dsum_rec();

    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      double p2 = n.deck[i-1];
      n.do_action(4, i, w);
      reward += ut_draw(n) * p1 * p2;
      n.undo_action(4, i, w);
    }

    return reward;
}

double ut_draw(node &n){
    if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) { 
      return -table_sign[n.turn];
    }
    //必勝判定
    if(use_good_move){
      if(n.open2 > 1 && n.barrier2 == false){
        if(n.hand1[0] == 1 || n.hand1[1] == 1){
          return table_sign[n.turn];
        }
      }
      if(n.open2 > 0 && n.barrier2 == false){
        if(n.hand1[0] == 3 && n.hand1[1] > n.open2){
          return table_sign[n.turn];
        }
        if(n.hand1[1] == 3 && n.hand1[0] > n.open2){
          return table_sign[n.turn];
        }
      }
      if(n.open2 == 8 && n.barrier2 == false){
        if(n.hand1[0] == 5 || n.hand1[1] == 5){
          return table_sign[n.turn];
        }
      }
    }
    work_do_action w;
    double r, r1, r2;
    //必敗判定
    if(nuse_bad_move){
      if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false){
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        r = ut_play(n, card);
        n.undo_action(5, card, w);
        return r;
      } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false){
        int card = n.hand1[0];
        n.do_action(5, 0, w);
        r = ut_play(n, card);
        n.undo_action(5, card, w);
        return r;
      } else if(n.hand1[0] == 8) {
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        r = ut_play(n, card);
        n.undo_action(5, card, w);
        return r;
      } else if(n.hand1[1] == 8){
        int card = n.hand1[0];
        n.do_action(5, 0, w);
        r = ut_play(n, card);
        n.undo_action(5, card, w);
        return r;
      }
    }
    //手札が同じカード2枚のとき
    if(use_same_move){
      if(n.hand1[0] == n.hand1[1]){
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        r = ut_play(n, card);
        n.undo_action(5, card, w);
        return r;
      }
    }
    
    int c1, c2;
    string key = n.rnd_his_p[n.turn].get_hash_value();
    w.infset_it = table_infset.find(key);
    assert(w.infset_it->second.get_sum_i(1) != 0);
    work_do_action w2 = w;
    double p = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
    if(w.infset_it->second.get_sum_i(1) == 0) p = 0.5;
    c1 = n.hand1[0]; c2 = n.hand1[1];
    n.do_action(5, 0, w);
    r1 = ut_play(n, c1);
    n.undo_action(5, c1, w);
    n.do_action(5, 1, w2);
    r2 = ut_play(n, c2);
    n.undo_action(5, c2, w2);
    r = r1 * p + r2 * (1 - p);
    return r;
}

double ut_play(node &n, int c){
    double reward;
    double prob_win;
    switch(c){
        case 1:
          if(n.open2 > 1 && n.barrier2 == false) {
            return table_sign[n.turn];
          }
          if(n.hand2 > 1 && n.barrier2 == false){
            double exist_card[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            for(int i = 0; i < 8; i++) exist_card[i] += n.deck[i];
            exist_card[n.hand2 - 1]++;
            exist_card[n.hide - 1]++;
            //オッズ変更はここで
            //exist_card[1] = exist_card[1] / 2.0;
            //exist_card[7] = exist_card[7] * 8.0;
            double sum_exist_card = exist_card[1] + exist_card[2] + exist_card[3] + exist_card[4] + exist_card[5] + exist_card[6] + exist_card[7];
            /*姫があるなら姫を必ず宣言するよう変更
            prob_win = exist_card[n.hand2 - 1] / sum_exist_card;
            if(exist_card[7] > 0){
              prob_win = 0.0;
              if(n.hand2 == 8) prob_win = 1.0;
            }*/
            if(n.dsum() == 0){
              if(n.hand1[0] > n.hand2){
                return table_sign[n.turn];
              } else if(n.hand1[0] < n.hand2){
                return prob_win * table_sign[n.turn] + (1 - prob_win) * (-table_sign[n.turn]);
              } else {
                return prob_win * table_sign[n.turn] + 0.0;
              }
            }   
            reward = prob_win * table_sign[n.turn];
            double p1 = n.dsum_rec();
            for(int i = 1; i < 9; i++){
              if(n.deck[i-1] == 0) { continue; }
              work_do_action ut_w;
              ut_w.randsol_wprob = prob_win;
              double p2 = n.deck[i-1];
              n.do_action(4, i, ut_w);
              reward += (1 - prob_win) * ut_draw(n) * p1 * p2;
              n.undo_action(4, i, ut_w);
            }
            return reward;
          }
          break;
        case 2:
          break;
        case 3:
          if(n.hand1[0] > n.hand2 && n.barrier2 == false) {
            return table_sign[n.turn]; 
          } else if (n.hand1[0] < n.hand2 && n.barrier2 == false){
            return -table_sign[n.turn];
          }
          break;
        case 4:
          break;
        case 5: {
            work_do_action ut_w0;
            string key = n.rnd_his_p[n.turn].get_hash_value();
            ut_w0.infset_it = table_infset.find(key);
            assert(ut_w0.infset_it->second.get_sum_i(1) != 0);
            double p = (double)ut_w0.infset_it->second.get_sum_i(0) / ut_w0.infset_it->second.get_sum_i(1);
            if(ut_w0.infset_it->second.get_sum_i(1) == 0) p = 0.5;
            double reward1 = 0.0;
            double reward2 = 0.0;
            double p1 = n.dsum_rec();
            if(n.dsum() == 0){
              if(n.barrier2 == false){
                if(n.hand2 == 8){
                  reward1 = table_sign[n.turn];
                } else if(n.hand1[0] > n.hide){
                  reward1 = table_sign[n.turn];
                } else if(n.hand1[0] < n.hide){
                  reward1 = -table_sign[n.turn];
                }
              } else {
                if(n.hand1[0] > n.hand2){
                  reward1 = table_sign[n.turn];
                } else if(n.hand1[0] < n.hand2){
                  reward1 = -table_sign[n.turn];
                }
              }
              if(n.hand1[0] == 8){
                reward2 = -table_sign[n.turn];
              } else if(n.hand2 > n.hide){
                reward2 = -table_sign[n.turn];
              } else if(n.hand2 < n.hide){
                reward2 = table_sign[n.turn];
              }
              reward = reward1 * p + reward2 * (1 - p);
              return reward;
            } else {
              if(n.barrier2 == false){
                if(n.hand2 == 8){
                  reward1 = table_sign[n.turn];
                } else {
                  for(int i = 1; i < 9; i++){
                    if(n.deck[i-1] == 0) { continue; }
                    work_do_action ut_w2 = ut_w0;
                    double p2 = n.deck[i-1];
                    n.do_action(6, i, ut_w2);
                    reward1 += ut_wizard(n) * p1 * p2;
                    n.undo_action(6, i, ut_w2);
                  }
                }
              } else {
                for(int i = 1; i < 9; i++){
                  if(n.deck[i-1] == 0) { continue; }
                  work_do_action ut_ww = ut_w0;
                  double p2 = n.deck[i-1];
                  n.do_action(6, i, ut_ww);
                  n.do_action(4, i, ut_ww);
                  reward1 += ut_draw(n) * p1 * p2;
                  n.undo_action(4, i, ut_ww);
                  n.undo_action(6, i, ut_ww);
                }
              }
              if(n.hand1[0] == 8){
                reward2 = -table_sign[n.turn];
              } else {
                for(int i = 1; i < 9; i++){
                  if(n.deck[i-1] == 0) { continue; }
                  work_do_action ut_w3 = ut_w0;
                  double p2 = n.deck[i-1];
                  n.do_action(7, i, ut_w3);
                  reward2 += ut_wizard_self(n) * p1 * p2;
                  n.undo_action(7, i, ut_w3);
                }
              }
              reward = reward1 * p + reward2 *(1 - p);
              return reward;
            }
          }
          break;
        case 6:
          break;
        case 7:
          break;
        case 8:
          return -table_sign[n.turn];
    }
    if(n.dsum() == 0){
      if(n.hand1[0] > n.hand2){
        return table_sign[n.turn];
      } else if(n.hand1[0] < n.hand2){
        return -table_sign[n.turn];
      } else {
        return 0.0;
      }
    }

    reward = 0.0;
    double p1 = n.dsum_rec();
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      double p2 = n.deck[i-1];
      n.do_action(4, i, w);
      reward += ut_draw(n) * p1 * p2;
      n.undo_action(4, i, w);
    }
    return reward;
}

double ut_wizard(node &n){
    if(n.dsum() == 0){
      if(n.hand1[0] > n.hand2){
        return table_sign[n.turn];
      } else if(n.hand1[0] < n.hand2){
        return -table_sign[n.turn];
      } else {
        return 0.0;
      }
    }

    double reward = 0.0;
    double p1 = n.dsum_rec();
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      double p2 = n.deck[i-1];
      n.do_action(4, i, w);
      reward += ut_draw(n) * p1 * p2;
      n.undo_action(4, i, w);
    }
    return reward;
}
double ut_wizard_self(node &n){
    if(n.dsum() == 0){
      if(n.hand1[0] > n.hand2){
        return table_sign[n.turn];
      } else if(n.hand1[0] < n.hand2){
        return -table_sign[n.turn];
      } else {
        return 0.0;
      }
    }

    double reward = 0.0;
    double p1 = n.dsum_rec();
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      double p2 = n.deck[i-1];
      n.do_action(4, i, w);
      reward += ut_draw(n) * p1 * p2;
      n.undo_action(4, i, w);
    }
    return reward;
}
