double infset_dfs_put_hide_card(node &n, vector<string> &m, int sign);
double infset_dfs_draw_p1_init(node &n, vector<string> &m, int sign);
double infset_dfs_draw_p2_init(node &n, vector<string> &m, int sign);
double infset_dfs_draw(node &n, vector<string> &m, int sign);
double infset_dfs_play(node &n, int c, vector<string> &m, int sign);
double infset_dfs_wizard(node &n, vector<string> &m, int sign);
double infset_dfs_wizard_self(node &n, vector<string> &m, int sign);
double infset_dfs_soldior(node &n, vector<string> &m, int sign);
void all_exp_reward(string his_p, int open[3], vector<string> &m, int sign, int next_sign);

double infset_dfs_put_hide_card(node &n, vector<string> &m, int sign) {
  double reward = 0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(2, i, w);
    reward += infset_dfs_draw_p1_init(n, m, sign) * p1 * p2;
    n.undo_action(2, i, w);
  }
  return reward;
}

double infset_dfs_draw_p1_init(node &n, vector<string> &m, int sign) {
  double reward = 0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(3, i, w);
    reward += infset_dfs_draw_p2_init(n, m, sign) * p1 * p2;
    n.undo_action(3, i, w);
  }
  return reward;
}

double infset_dfs_draw_p2_init(node &n, vector<string> &m, int sign) {
  double reward = 0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += infset_dfs_draw(n, m, sign) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}

double infset_dfs_draw(node &n, vector<string> &m, int sign) {
  if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) {
    return table_sign[1 - n.turn];
  }
  //必勝判定
  if(use_good_move) {
    if(n.open2 > 1 && n.barrier2 == false) {
      if(n.hand1[0] == 1 || n.hand1[1] == 1) {
        return table_sign[n.turn];
      }
    }
    if(n.open2 > 0 && n.barrier2 == false) {
      if(n.hand1[0] == 3 && n.hand1[1] > n.open2) {
        return table_sign[n.turn];
      }
      if(n.hand1[1] == 3 && n.hand1[0] > n.open2) {
        return table_sign[n.turn];
      }
    }
    if(n.open2 == 8 && n.barrier2 == false) {
      if(n.hand1[0] == 5 || n.hand1[1] == 5) {
        return table_sign[n.turn];
      }
    }
  }
  work_do_action w;
  double reward = 0;
  //必敗判定
  if(nuse_bad_move) {
    if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) {
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      reward = infset_dfs_play(n, card, m, sign);
      n.undo_action(5, card, w);
      return reward;
    } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) {
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      reward = infset_dfs_play(n, card, m, sign);
      n.undo_action(5, card, w);
      return reward;
    } else if(n.hand1[0] == 8) {
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      reward = infset_dfs_play(n, card, m, sign);
      n.undo_action(5, card, w);
      return reward;
    } else if(n.hand1[1] == 8) {
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      reward = infset_dfs_play(n, card, m, sign);
      n.undo_action(5, card, w);
      return reward;
    }
  }
  //手札が同じカード2枚のとき
  if(use_same_move) {
    if(n.hand1[0] == n.hand1[1]) {
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      reward = infset_dfs_play(n, card, m, sign);
      n.undo_action(5, card, w);
      return reward;
    }
  }

  int c1, c2;
  string key;
  if(n.turn == br_player) {
    map<string, double>::iterator table_exp_reward_it;
    table_exp_reward_it = table_exp_reward.find(n.org_his.get_hash_value());
    if(table_exp_reward_it == table_exp_reward.end()) {
      key = n.org_his_p[br_player].get_hash_value();
      all_exp_reward(key, n.open, m, sign, 0);
      table_exp_reward_it = table_exp_reward.find(n.org_his.get_hash_value());
      if(table_exp_reward_it == table_exp_reward.end()) {
        cout << "all_exp_reward fault : draw" << endl;
        output_hash_history(n.org_his.get_hash_value(), false);
        cout << endl;
        terminate();
      }
    }
    reward = table_exp_reward_it->second;
    table_exp_reward.erase(table_exp_reward_it);
  } else {
    key = n.rnd_his_p[n.turn].get_hash_value();
    w.infset_it = table_infset.find(key);
    if(w.infset_it == table_infset.end()) {
      cout << "infset_dfs_draw : infset not found." << endl;
      terminate();
    }
    double p = w.infset_it->second.get_prob_action();
    double r1 = 0;
    double r2 = 0;
    work_do_action w2 = w;
    c1 = n.hand1[0];
    c2 = n.hand1[1];
    n.do_action(5, 0, w);
    r1 = infset_dfs_play(n, c1, m, sign);
    n.undo_action(5, c1, w);
    n.do_action(5, 1, w2);
    r2 = infset_dfs_play(n, c2, m, sign);
    n.undo_action(5, c2, w2);
    reward = r1 * p + r2 * (1.0 - p);
  }
  return reward;
}

double infset_dfs_play(node &n, int c, vector<string> &m, int sign) {
  double reward = 0.0;
  switch(c) {
  case 1:
    if(n.open2 > 1 && n.barrier2 == false) {
      return table_sign[n.turn];
    }
    if(n.barrier2 == false) {
      if(n.turn == br_player) {
        map<string, double>::iterator table_exp_reward_it;
        table_exp_reward_it = table_exp_reward.find(n.org_his.get_hash_value());
        if(table_exp_reward_it == table_exp_reward.end()) {
          string key = n.org_his_p[br_player].get_hash_value();
          all_exp_reward(key, n.open, m, sign, 1);
          table_exp_reward_it = table_exp_reward.find(n.org_his.get_hash_value());
          if(table_exp_reward_it == table_exp_reward.end()) {
            cout << "all_exp_reward fault : play" << endl;
            terminate();
          }
        }
        reward = table_exp_reward_it->second;
        table_exp_reward.erase(table_exp_reward_it);
        return reward;
      } else {
        double exist_card[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        for(int i = 0; i < 8; i++) exist_card[i] += n.deck[i];
        exist_card[n.hand2 - 1]++;
        exist_card[n.hide - 1]++;
        // exist_card[1] = exist_card[1] / 2.0;
        // exist_card[7] = exist_card[7] * 8.0;
        double sum_exist_card = exist_card[1] + exist_card[2] + exist_card[3] + exist_card[4] + exist_card[5] + exist_card[6] + exist_card[7];
        if(sum_exist_card != 0) {
          soldior_points++;
          for(int i = 1; i < 8; i++) random_soldior_prob[i - 1] += exist_card[i] / sum_exist_card;
        }

        double prob_win = 0;
        if(n.hand2 > 1) {
          prob_win = exist_card[n.hand2 - 1] / sum_exist_card;
          if(exist_card[7] > 0) {
            prob_win = 0.0;
            if(n.hand2 == 8) prob_win = 1.0;
          }
          /*
          if(n.hide != 1){
            prob_win = n.deck[1] + n.deck[2] + n.deck[3] + n.deck[4] + n.deck[5] + n.deck[6] + n.deck[7] + 2;
            if(n.hide != n.hand2){
              prob_win = (1 + n.deck[n.hand2 - 1]) / prob_win;
            } else {
              prob_win = (1 + 1) / prob_win;
            }
          } else {
            prob_win = n.deck[1] + n.deck[2] + n.deck[3] + n.deck[4] + n.deck[5] + n.deck[6] + n.deck[7] + 1;
            prob_win = (1 + n.deck[n.hand2 - 1]) / prob_win;
          }
          */
          if(n.dsum() == 0) {
            if(n.hand1[0] > n.hand2) {
              return table_sign[n.turn];
            } else if(n.hand1[0] < n.hand2) {
              return prob_win * table_sign[n.turn] + (1.0 - prob_win) * (table_sign[1 - n.turn]);
            } else {
              return prob_win * table_sign[n.turn] + 0.0;
            }
          }
          reward = prob_win * table_sign[n.turn];
        } else {
          if(n.dsum() == 0) {
            if(n.hand1[0] > n.hand2) {
              return table_sign[n.turn];
            } else if(n.hand1[0] < n.hand2) {
              return table_sign[1 - n.turn];
            } else {
              return 0.0;
            }
          }
        }
        double p1 = n.dsum_rec();
        for(int i = 1; i < 9; i++) {
          if(n.deck[i - 1] == 0) {
            continue;
          }
          work_do_action exp_w;
          exp_w.randsol_wprob = prob_win;
          double p2 = n.deck[i - 1];
          n.do_action(4, i, exp_w);
          reward += (1 - prob_win) * infset_dfs_draw(n, m, sign) * p1 * p2;
          n.undo_action(4, i, exp_w);
        }
        return reward;
      }
    }
    break;
  case 2:
    break;
  case 3:
    if(n.hand1[0] > n.hand2 && n.barrier2 == false) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2 && n.barrier2 == false) {
      return table_sign[1 - n.turn];
    }
    break;
  case 4:
    break;
  case 5: {
    if(n.turn == br_player) {
      map<string, double>::iterator table_exp_reward_it;
      table_exp_reward_it = table_exp_reward.find(n.org_his.get_hash_value());
      if(table_exp_reward_it == table_exp_reward.end()) {
        string key = n.org_his_p[br_player].get_hash_value();
        all_exp_reward(key, n.open, m, sign, 5);
        table_exp_reward_it = table_exp_reward.find(n.org_his.get_hash_value());
        if(table_exp_reward_it == table_exp_reward.end()) {
          cout << "all_exp_reward fault : wizard" << endl;
          terminate();
        }
      }
      reward = table_exp_reward_it->second;
      table_exp_reward.erase(table_exp_reward_it);
      return reward;
    } else {
      work_do_action exp_w0;
      exp_w0.infset_it = table_infset.find(n.rnd_his_p[n.turn].get_hash_value());
      if(exp_w0.infset_it == table_infset.end()) {
        cout << "infset_dfs_play : infset not found." << endl;
        terminate();
      }
      double p = exp_w0.infset_it->second.get_prob_action();
      double reward1 = 0.0;
      double reward2 = 0.0;
      double p1 = n.dsum_rec();
      if(n.dsum() == 0) {
        if(n.barrier2 == false) {
          if(n.hand2 == 8) {
            reward1 = table_sign[n.turn];
          } else if(n.hand1[0] > n.hide) {
            reward1 = table_sign[n.turn];
          } else if(n.hand1[0] < n.hide) {
            reward1 = table_sign[1 - n.turn];
          }
        } else {
          if(n.hand1[0] > n.hand2) {
            reward1 = table_sign[n.turn];
          } else if(n.hand1[0] < n.hand2) {
            reward1 = table_sign[1 - n.turn];
          }
        }
        if(n.hand1[0] == 8) {
          reward2 = table_sign[1 - n.turn];
        } else if(n.hand2 > n.hide) {
          reward2 = table_sign[1 - n.turn];
        } else if(n.hand2 < n.hide) {
          reward2 = table_sign[n.turn];
        }
        reward = reward1 * p + reward2 * (1 - p);
        return reward;
      } else {
        if(n.barrier2 == false) {
          if(n.hand2 == 8) {
            reward1 = table_sign[n.turn];
          } else {
            for(int i = 1; i < 9; i++) {
              if(n.deck[i - 1] == 0) {
                continue;
              }
              work_do_action exp_w2 = exp_w0;
              double p2 = n.deck[i - 1];
              n.do_action(6, i, exp_w2);
              reward1 += infset_dfs_wizard(n, m, sign) * p1 * p2;
              n.undo_action(6, i, exp_w2);
            }
          }
        } else {
          work_do_action exp_ww = exp_w0;
          n.do_action(6, 0, exp_ww);
          reward1 = infset_dfs_wizard(n, m, sign);
          n.undo_action(6, 0, exp_ww);
        }
        if(n.hand1[0] == 8) {
          reward2 = table_sign[1 - n.turn];
        } else {
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            work_do_action exp_w3 = exp_w0;
            double p2 = n.deck[i - 1];
            n.do_action(7, i, exp_w3);
            reward2 += infset_dfs_wizard_self(n, m, sign) * p1 * p2;
            n.undo_action(7, i, exp_w3);
          }
        }
        reward = reward1 * p + reward2 * (1 - p);
        return reward;
      }
    }
  } break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    return table_sign[1 - n.turn];
  }
  if(n.dsum() == 0) {
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return table_sign[1 - n.turn];
    } else {
      return 0.0;
    }
  }

  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += infset_dfs_draw(n, m, sign) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}

double infset_dfs_wizard(node &n, vector<string> &m, int sign) {
  if(n.dsum() == 0) {
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return table_sign[1 - n.turn];
    } else {
      return 0.0;
    }
  }

  double reward = 0.0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += infset_dfs_draw(n, m, sign) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}
double infset_dfs_wizard_self(node &n, vector<string> &m, int sign) {
  if(n.dsum() == 0) {
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return table_sign[1 - n.turn];
    } else {
      return 0.0;
    }
  }

  double reward = 0.0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += infset_dfs_draw(n, m, sign) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}
double infset_dfs_soldior(node &n, vector<string> &m, int sign) {
  if(n.dsum() == 0) {
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return table_sign[1 - n.turn];
    } else {
      return 0.0;
    }
  }

  double reward = 0.0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += infset_dfs_draw(n, m, sign) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}

void all_exp_reward(string his_p, int open[3], vector<string> &m, int sign, int next_sign) {
  // output_hash_history(his_p, false); cout << " : " << sign << ", " << next_sign << endl;
  all_history.clear();
  if(m.empty()) {
    node n(open);
    for(int i = 1; i < 9; i++) {
      if(n.deck[i - 1] == 0) {
        continue;
      }
      work_do_action all_w;
      n.do_action(1, i, all_w);
      all_put_hide_card(his_p, 0, n);
      n.undo_action(1, i, all_w);
    }
  } else {
    for(vector<string>::iterator it = m.begin(); it != m.end(); it++) {
      string g = *it;
      node n(g, false, open);

      work_do_action w;
      unsigned long int head = n.org_his_p[n.turn].get_hash_value().size();
      if(head > his_p.size()) {
        cout << "his_p : ";
        output_hash_history(his_p, false);
        cout << " n.his_p : ";
        output_hash_history(n.org_his_p[br_player].get_hash_value(), false);
        cout << " sign : " << sign << " next_sign : " << next_sign << endl;
        n.print_node();
        terminate();
      }
      if(head == 0) sign = -1;
      if(sign == 0) {
        w.infset_it = table_infset.begin();
        int c1 = n.hand1[0];
        int c2 = n.hand1[1];
        string action = oph.get_action((unsigned char)his_p[head]);
        int c2a = char_to_action(action[0]);
        int c2a1 = c2a / 10;
        int c2a2 = c2a % 10;
        work_do_action w2 = w;
        if(c2a1 == 4) {
          if(c2a2 + 1 == n.hand1[1]) {
            n.do_action(5, 1, w2);
            all_play(his_p, head, n, c2);
            n.undo_action(5, c2, w2);
          } else if(c2a2 + 1 == n.hand1[0]) {
            n.do_action(5, 0, w);
            all_play(his_p, head, n, c1);
            n.undo_action(5, c1, w);
          } else {
            continue;
          }
        } else {
          cout << "terminate : play" << endl;
          terminate();
        }
      } else if(sign == 1) { // cout << "all_exp_reward : " << sign << endl;
        head--;
        string action = oph.get_action((unsigned char)his_p[head]);
        int c2a = char_to_action(action[0]);
        int c2a1 = c2a / 10;
        if(c2a1 != 4) {
          output_hash_history(n.org_his.get_hash_value(), false);
          cout << "terminate : soldior" << endl;
          terminate();
        }
        if(action.size() > 1) {
          int c2t = char_to_twonum(action[1]);
          int choice = c2t % 10;
          if(choice + 1 != 1 && choice + 1 == n.hand2) continue;
          bool candidate[8] = {false, false, false, false, false, false, false, false};
          candidate[n.hide - 1] = true;
          candidate[n.hand2 - 1] = true;
          for(int i = 2; i < 9; i++) {
            if(n.deck[i - 1] > 0) candidate[i - 1] = true;
            if(!candidate[i - 1]) continue;
            if(i == choice + 1) {
              work_do_action all_ws;
              head++;
              n.do_action(8, i, all_ws);
              all_soldior(his_p, head, n);
              n.undo_action(8, i, all_ws);
              head--;
            }
          }
          if(!candidate[1] && !candidate[2] && !candidate[3] && !candidate[4] && !candidate[5] && !candidate[6] && !candidate[7]) {
            head++;
            all_soldior(his_p, head, n);
            head--;
          }
        } else {
          head++;
          all_soldior(his_p, head, n);
          head--;
        }
      } else if(sign == 5) { // cout << "all_exp_reward : " << sign << endl;
        head--;
        w.infset_it = table_infset.begin();
        string action = oph.get_action((unsigned char)his_p[head]);
        int c2a = char_to_action(action[0]);
        int c2a1 = c2a / 10;
        if(c2a1 != 4) {
          cout << "terminate : wizard" << endl;
          terminate();
        }
        int c2w = char_to_wizard(action[1]);
        int to = c2w / 100;
        int trash = (c2w / 10) % 10;
        int draw = c2w % 10;
        action = oph.get_action((unsigned char)his_p[0]);
        int c2a0 = char_to_action(action[0]);
        int c2a01 = c2a0 / 10;
        if((c2a01 == 1 && to == 1 && n.turn == 0) || (c2a01 == 2 && to == 0 && n.turn == 1)) {
          if(n.barrier2 == false) {
            if(n.hand2 != trash + 1) continue;
            if(n.hand2 == 8) continue;
            for(int i = 1; i < 9; i++) {
              if(n.deck[i - 1] == 0) {
                continue;
              }
              work_do_action all_w2 = w;
              head++;
              n.do_action(6, i, all_w2);
              all_wizard(his_p, head, n);
              n.undo_action(6, i, all_w2);
              head--;
            }
          } else {
            work_do_action all_ww = w;
            head++;
            n.do_action(6, 0, all_ww);
            all_wizard(his_p, head, n);
            n.undo_action(6, 0, all_ww);
            head--;
          }
        } else if((c2a01 == 1 && to == 0 && n.turn == 1) || (c2a01 == 2 && to == 1 && n.turn == 0)) {
          if(n.barrier2 == false) {
            if(n.hand2 != trash + 1) continue;
            if(n.deck[draw] == 0) continue;
            if(n.hand2 == 8) continue;
            work_do_action all_w3 = w;
            head++;
            n.do_action(6, draw + 1, all_w3);
            all_wizard_self(his_p, head, n);
            n.undo_action(6, draw + 1, all_w3);
            head--;
          } else {
            head++;
            string action = oph.get_action((unsigned char)his_p[head]);
            int c2a = char_to_action(action[0]);
            int ctoa2_a = c2a / 10;
            int ctoa2_b = c2a % 10;
            if(ctoa2_a == 3) {
              if(n.deck[ctoa2_b] == 0) {
                continue;
              }
              work_do_action all_ww = w;
              head++;
              n.do_action(6, 1, all_ww);
              n.do_action(4, ctoa2_b + 1, all_ww);
              all_draw(his_p, head, n);
              n.undo_action(4, ctoa2_b + 1, all_ww);
              n.undo_action(6, 1, all_ww);
              head--;
            } else {
              work_do_action all_ww = w;
              head++;
              n.do_action(6, 0, all_ww);
              all_wizard(his_p, head, n);
              n.undo_action(6, 0, all_ww);
              head--;
            }
          }
        } else if((c2a01 == 1 && to == 1 && n.turn == 1) || (c2a01 == 2 && to == 0 && n.turn == 0)) {
          if(n.hand1[0] != trash + 1) continue;
          if(n.hand1[0] == 8) continue;
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            work_do_action all_w2 = w;
            head++;
            n.do_action(7, i, all_w2);
            all_wizard_self(his_p, head, n);
            n.undo_action(7, i, all_w2);
            head--;
          }
        } else {
          if(n.hand1[0] != trash + 1) continue;
          if(n.deck[draw] == 0) continue;
          if(n.hand1[0] == 8) continue;
          work_do_action all_w3 = w;
          head++;
          n.do_action(7, draw + 1, all_w3);
          all_wizard_self(his_p, head, n);
          n.undo_action(7, draw + 1, all_w3);
          head--;
        }
      } else {
        // cout << "all_exp_reward : " << sign << endl;
        node n_all(n.open);
        for(int i = 1; i < 9; i++) {
          if(n_all.deck[i - 1] == 0) {
            continue;
          }
          work_do_action all_w;
          n_all.do_action(1, i, all_w);
          all_put_hide_card(his_p, 0, n_all);
          n_all.undo_action(1, i, all_w);
        }
      }
    }
  }
  vector<string> mem_all_history = all_history;

  /*output_hash_history(his_p, false); cout << " : " << sign << ", " << next_sign << endl;
  for(std::vector<std::string>::iterator h = mem_all_history.begin(); h != mem_all_history.end(); ++h){
    output_hash_history(*h, false);
    cout << endl;
  }*/

  double reward_sum[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  bool candidate_all[8] = {false, false, false, false, false, false, false, false};
  double reward_all[mem_all_history.size()][8];
  for(unsigned int i = 0; i < mem_all_history.size(); i++) {
    for(int j = 0; j < 8; j++) {
      reward_all[i][j] = 0.0;
    }
  }
  int his_index = 0;
  for(std::vector<std::string>::iterator h = mem_all_history.begin(); h != mem_all_history.end(); ++h) {
    string g = *h;
    node n_exp(g, false, open);
    double po = n_exp.npi[1 - n_exp.turn][n_exp.depth];
    double pr = n_exp.npi[2][n_exp.depth];
    work_do_action w, w2;
    switch(next_sign) {
    case 0: {
      w.infset_it = table_infset.begin();
      w2 = w;
      int c1 = n_exp.hand1[0];
      int c2 = n_exp.hand1[1];
      n_exp.do_action(5, 0, w);
      reward_all[his_index][0] = infset_dfs_play(n_exp, c1, mem_all_history, next_sign);
      n_exp.undo_action(5, c1, w);
      n_exp.do_action(5, 1, w2);
      reward_all[his_index][1] = infset_dfs_play(n_exp, c2, mem_all_history, next_sign);
      n_exp.undo_action(5, c2, w2);
      reward_sum[0] += reward_all[his_index][0] * po * pr;
      reward_sum[1] += reward_all[his_index][1] * po * pr;
      break;
    }
    case 1: {
      if(n_exp.barrier2 == false) {
        soldior_infsets++;
        bool candidate[8] = {false, false, false, false, false, false, false, false};
        candidate[n_exp.hide - 1] = true;
        candidate[n_exp.hand2 - 1] = true;
        for(int i = 2; i < 9; i++) {
          if(n_exp.deck[i - 1] > 0) candidate[i - 1] = true;
          if(!candidate[i - 1]) continue;
          if(i == n_exp.hand2) {
            reward_all[his_index][i - 1] = table_sign[n_exp.turn];
          } else {
            n_exp.do_action(8, i, w);
            reward_all[his_index][i - 1] = infset_dfs_soldior(n_exp, mem_all_history, next_sign);
            n_exp.undo_action(8, i, w);
          }
          reward_sum[i - 1] += reward_all[his_index][i - 1] * po * pr;
          candidate_all[i - 1] = true;
        }
        if(!candidate[1] && !candidate[2] && !candidate[3] && !candidate[4] && !candidate[5] && !candidate[6] && !candidate[7]) {
          reward_all[his_index][0] = infset_dfs_soldior(n_exp, mem_all_history, next_sign);
          reward_sum[0] += reward_all[his_index][0] * po * pr;
          candidate_all[0] = true;
        }
      } else {
        reward_all[his_index][0] = infset_dfs_soldior(n_exp, mem_all_history, next_sign);
        reward_sum[0] += reward_all[his_index][0] * po * pr;
        candidate_all[0] = true;
      }
      break;
    }
    case 5:
      w.infset_it = table_infset.begin();
      w2 = w;
      if(n_exp.dsum() == 0) {
        if(n_exp.barrier2 == false) {
          if(n_exp.hand2 == 8) {
            reward_all[his_index][0] = table_sign[n_exp.turn];
          } else if(n_exp.hand1[0] > n_exp.hide) {
            reward_all[his_index][0] = table_sign[n_exp.turn];
          } else if(n_exp.hand1[0] < n_exp.hide) {
            reward_all[his_index][0] = table_sign[1 - n_exp.turn];
          } else {
            reward_all[his_index][0] = 0.0;
          }
        } else {
          if(n_exp.hand1[0] > n_exp.hand2) {
            reward_all[his_index][0] = table_sign[n_exp.turn];
          } else if(n_exp.hand1[0] < n_exp.hand2) {
            reward_all[his_index][0] = table_sign[1 - n_exp.turn];
          } else {
            reward_all[his_index][0] = 0.0;
          }
        }
        if(n_exp.hand1[0] == 8) {
          reward_all[his_index][1] = table_sign[1 - n_exp.turn];
        } else if(n_exp.hand2 > n_exp.hide) {
          reward_all[his_index][1] = table_sign[1 - n_exp.turn];
        } else if(n_exp.hand2 < n_exp.hide) {
          reward_all[his_index][1] = table_sign[n_exp.turn];
        } else {
          reward_all[his_index][1] = 0.0;
        }
      } else {
        if(n_exp.barrier2 == false) {
          if(n_exp.hand2 == 8) {
            reward_all[his_index][0] = table_sign[n_exp.turn];
          } else {
            double p1 = n_exp.dsum_rec();
            for(int i = 1; i < 9; i++) {
              if(n_exp.deck[i - 1] == 0) {
                continue;
              }
              double p2 = n_exp.deck[i - 1];
              work_do_action exp_w = w;
              n_exp.do_action(6, i, exp_w);
              reward_all[his_index][0] += infset_dfs_wizard(n_exp, mem_all_history, next_sign) * p1 * p2;
              n_exp.undo_action(6, i, exp_w);
            }
          }
        } else {
          work_do_action exp_ww = w;
          n_exp.do_action(6, 0, exp_ww);
          reward_all[his_index][0] = infset_dfs_wizard(n_exp, mem_all_history, next_sign);
          n_exp.undo_action(6, 0, exp_ww);
        }
        if(n_exp.hand1[0] == 8) {
          reward_all[his_index][1] = table_sign[1 - n_exp.turn];
        } else {
          double p1 = n_exp.dsum_rec();
          for(int i = 1; i < 9; i++) {
            if(n_exp.deck[i - 1] == 0) {
              continue;
            }
            work_do_action exp_w = w;
            double p2 = n_exp.deck[i - 1];
            n_exp.do_action(7, i, exp_w);
            reward_all[his_index][1] += infset_dfs_wizard_self(n_exp, mem_all_history, next_sign) * p1 * p2;
            n_exp.undo_action(7, i, exp_w);
          }
        }
      }
      reward_sum[0] += reward_all[his_index][0] * po * pr;
      reward_sum[1] += reward_all[his_index][1] * po * pr;
      break;
    }
    his_index++;
  }
  int maxexp_index = 8;
  if(br_player == 0) {
    if(next_sign == 1) {
      double maxexp = -1.0;
      for(int i = 0; i < 8; i++) {
        if(candidate_all[i] && reward_sum[i] > maxexp) {
          maxexp = reward_sum[i];
          maxexp_index = i;
        }
      }
    } else {
      if(reward_sum[0] > reward_sum[1]) {
        maxexp_index = 0;
      } else {
        maxexp_index = 1;
      }
    }
  } else {
    if(next_sign == 1) {
      double minexp = 1.0;
      for(int i = 0; i < 8; i++) {
        if(candidate_all[i] && reward_sum[i] < minexp) {
          minexp = reward_sum[i];
          maxexp_index = i;
        }
      }
    } else {
      if(reward_sum[0] < reward_sum[1]) {
        maxexp_index = 0;
      } else {
        maxexp_index = 1;
      }
    }
  }
  his_index = 0;
  for(std::vector<std::string>::iterator h = mem_all_history.begin(); h != mem_all_history.end(); ++h) {
    table_exp_reward[*h] = reward_all[his_index][maxexp_index];
    // output_hash_history(*h, false); cout << " : " << reward_all[his_index][maxexp_index] << endl;
    his_index++;
  }

  return;
}