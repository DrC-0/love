enum game_tree_mode {
  MRND_DS,
  MCFR,
  MCFR_EXP_REWARD,
};
extern game_tree_mode g;
double put_hide_card(node &n);
double draw_p1_init(node &n);
double draw_p2_init(node &n);
double draw(node &n);
double play(node &n, int c);
double wizard(node &n);
double wizard_self(node &n);

double put_hide_card(node &n) {
  double reward = 0.0;
  double p1 = n.dsum_rec();
  if(g == MRND_DS) {
    rand_points++;
  }

  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(2, i, w);
    reward += draw_p1_init(n) * p1 * p2;
    n.undo_action(2, i, w);
  }

  return reward;
}

double draw_p1_init(node &n) {
  double reward = 0.0;
  double p1 = n.dsum_rec();
  if(g == MRND_DS) {
    rand_points++;
  }

  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(3, i, w);
    reward += draw_p2_init(n) * p1 * p2;
    n.undo_action(3, i, w);
  }

  return reward;
}

double draw_p2_init(node &n) {
  double reward = 0.0;
  double p1 = n.dsum_rec();
  if(g == MRND_DS) {
    rand_points++;
  }

  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += draw(n) * p1 * p2;
    n.undo_action(4, i, w);
  }

  return reward;
}

double draw(node &n) {
  if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) {
    if(g == MRND_DS) end_points++;
    return -table_sign[n.turn];
  }
  //必勝判定
  if(use_good_move) {
    if(n.open2 > 1 && n.barrier2 == false) {
      if(n.hand1[0] == 1 || n.hand1[1] == 1) {
        if(g == MRND_DS) end_points++;
        return table_sign[n.turn];
      }
    }
    if(n.open2 > 0 && n.barrier2 == false) {
      if(n.hand1[0] == 3 && n.hand1[1] > n.open2) {
        if(g == MRND_DS) end_points++;
        return table_sign[n.turn];
      }
      if(n.hand1[1] == 3 && n.hand1[0] > n.open2) {
        if(g == MRND_DS) end_points++;
        return table_sign[n.turn];
      }
    }
    if(n.open2 == 8 && n.barrier2 == false) {
      if(n.hand1[0] == 5 || n.hand1[1] == 5) {
        if(g == MRND_DS) end_points++;
        return table_sign[n.turn];
      }
    }
  }
  work_do_action w;
  double r, r1, r2;
  //必敗判定
  if(nuse_bad_move) {
    if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) {
      if(g == MRND_DS) end_points++;
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      r = play(n, card);
      n.undo_action(5, card, w);
      return r;
    } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) {
      if(g == MRND_DS) end_points++;
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      r = play(n, card);
      n.undo_action(5, card, w);
      return r;
    } else if(n.hand1[0] == 8) {
      if(g == MRND_DS) end_points++;
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      r = play(n, card);
      n.undo_action(5, card, w);
      return r;
    } else if(n.hand1[1] == 8) {
      if(g == MRND_DS) end_points++;
      int card = n.hand1[0];
      n.do_action(5, 0, w);
      r = play(n, card);
      n.undo_action(5, card, w);
      return r;
    }
  }
  //手札が同じカード2枚のとき
  if(use_same_move) {
    if(n.hand1[0] == n.hand1[1]) {
      int card = n.hand1[1];
      n.do_action(5, 1, w);
      r = play(n, card);
      n.undo_action(5, card, w);
      return r;
    }
  }
  if(g == MRND_DS) {
    if(n.turn == 0) {
      p1_points++;
    } else {
      p2_points++;
    }
  }

  int c1, c2;
  string key = n.rnd_his_p[n.turn].get_hash_value();
  if(g == MRND_DS) {
    infset &x = table_infset[key];
    if(x.get_pi_i() > 0 && abs(x.get_pi_i() - n.npi[n.turn][n.depth]) > pow(10.0, -10.0)) terminate();
    x.set_pi_i(n.npi[n.turn][n.depth]);
    assert(x.get_pi_i() != 0);
    x.depth = n.count_turn;
    x.play = true;
  }
  w.infset_it = table_infset.find(key);
  work_do_action w2 = w;
  double p = 0;

  if(g == MCFR) {
    p = w.infset_it->second.get_prob_action();
  } else if(g == MCFR_EXP_REWARD) {
    assert(w.infset_it->second.get_sum_i(1) != 0);
    p = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
    if(w.infset_it->second.get_sum_i(1) == 0) p = 0.5;
  }

  c1 = n.hand1[0];
  c2 = n.hand1[1];
  n.do_action(5, 0, w);
  r1 = play(n, c1);
  n.undo_action(5, c1, w);
  n.do_action(5, 1, w2);
  r2 = play(n, c2);
  n.undo_action(5, c2, w2);
  r = r1 * p + r2 * (1 - p);

  if(g == MCFR) {
    w.infset_it->second.set_regret(0, w.infset_it->second.get_regret(0) + n.npi[1 - n.turn][n.depth] * n.npi[2][n.depth] * (r1 - r));
    w.infset_it->second.set_regret(1, w.infset_it->second.get_regret(1) + n.npi[1 - n.turn][n.depth] * n.npi[2][n.depth] * (r2 - r));
  }

  return r;
}

double play(node &n, int c) {
  double reward;
  double prob_win = 0.0;
  switch(c) {
  case 1:
    if(n.open2 > 1 && n.barrier2 == false) {
      if(g == MRND_DS) end_points++;
      return table_sign[n.turn];
    }
    if(n.hand2 > 1 && n.barrier2 == false) {
      if(g == MRND_DS) {
        end_points++;
        if(n.dsum() == 0) {
          end_points++;
          return 0.0;
        }
      }
      double exist_card[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
      for(int i = 0; i < 8; i++) exist_card[i] += n.deck[i];
      exist_card[n.hand2 - 1]++;
      exist_card[n.hide - 1]++;
      //オッズ変更はここで
      // exist_card[1] = exist_card[1] / 2.0;
      // exist_card[7] = exist_card[7] * 8.0;
      double sum_exist_card = exist_card[1] + exist_card[2] + exist_card[3] + exist_card[4] + exist_card[5] + exist_card[6] + exist_card[7];
      prob_win = exist_card[n.hand2 - 1] / sum_exist_card;
      //姫があるなら姫を必ず宣言するよう変更
      // if(exist_card[7] > 0){
      //   prob_win = 0.0;
      //   if(n.hand2 == 8) prob_win = 1.0;
      // }

      if(g == MRND_DS) {
        if(n.hide != 1) {
          prob_win = n.deck[1] + n.deck[2] + n.deck[3] + n.deck[4] + n.deck[5] + n.deck[6] + n.deck[7] + 2;
          if(n.hide != n.hand2) {
            prob_win = (1 + n.deck[n.hand2 - 1]) / prob_win;
          } else {
            prob_win = (1 + 1) / prob_win;
          }
        } else {
          prob_win = n.deck[1] + n.deck[2] + n.deck[3] + n.deck[4] + n.deck[5] + n.deck[6] + n.deck[7] + 1;
          prob_win = (1 + n.deck[n.hand2 - 1]) / prob_win;
        }
      }
      if(n.dsum() == 0) {
        if(g == MRND_DS) {
          end_points++;
          return 0.0;
        }
        if(n.hand1[0] > n.hand2) {
          return table_sign[n.turn];
        } else if(n.hand1[0] < n.hand2) {
          return prob_win * table_sign[n.turn] + (1 - prob_win) * (-table_sign[n.turn]);
        } else {
          return prob_win * table_sign[n.turn] + 0.0;
        }
      }
      if(g == MRND_DS) {
        rand_points++;
      }
      reward = prob_win * table_sign[n.turn];
      double p1 = n.dsum_rec();
      for(int i = 1; i < 9; i++) {
        if(n.deck[i - 1] == 0) {
          continue;
        }
        work_do_action w1;
        w1.randsol_wprob = prob_win;
        double p2 = n.deck[i - 1];
        n.do_action(4, i, w1);
        reward += (1 - prob_win) * draw(n) * p1 * p2;
        n.undo_action(4, i, w1);
      }
      return reward;
    }
    break;
  case 2:
    break;
  case 3:
    if(n.hand1[0] > n.hand2 && n.barrier2 == false) {
      if(g == MRND_DS) end_points++;
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2 && n.barrier2 == false) {
      if(g == MRND_DS) end_points++;
      return -table_sign[n.turn];
    }
    break;
  case 4:
    break;
  case 5: {
    if(g == MRND_DS) {
      if(n.turn == 0) {
        p1_points++;
      } else {
        p2_points++;
      }
    }
    work_do_action w0;
    string key = n.rnd_his_p[n.turn].get_hash_value();
    if(g == MRND_DS) {
      infset &x = table_infset[key];
      if(x.get_pi_i() > 0 && abs(x.get_pi_i() - n.npi[n.turn][n.depth]) > pow(10.0, -10.0)) terminate();
      x.set_pi_i(n.npi[n.turn][n.depth]);
      assert(x.get_pi_i() != 0);
      x.depth = n.count_turn;
      x.wizard = true;
    }
    w0.infset_it = table_infset.find(key);
    double p = 0;

    if(g == MCFR) {
      p = w0.infset_it->second.get_prob_action();
    } else if(g == MCFR_EXP_REWARD) {
      assert(w0.infset_it->second.get_sum_i(1) != 0);
      p = (double)w0.infset_it->second.get_sum_i(0) / w0.infset_it->second.get_sum_i(1);
      if(w0.infset_it->second.get_sum_i(1) == 0) p = 0.5;
    }

    double reward1 = 0.0;
    double reward2 = 0.0;
    double p1 = n.dsum_rec();
    if(n.dsum() == 0) {
      if(g == MRND_DS) {
        end_points++;
        return 0.0;
      }
      if(n.barrier2 == false) {
        if(n.hand2 == 8) {
          reward1 = table_sign[n.turn];
        } else if(n.hand1[0] > n.hide) {
          reward1 = table_sign[n.turn];
        } else if(n.hand1[0] < n.hide) {
          reward1 = -table_sign[n.turn];
        }
      } else {
        if(n.hand1[0] > n.hand2) {
          reward1 = table_sign[n.turn];
        } else if(n.hand1[0] < n.hand2) {
          reward1 = -table_sign[n.turn];
        }
      }
      if(n.hand1[0] == 8) {
        reward2 = -table_sign[n.turn];
      } else if(n.hand2 > n.hide) {
        reward2 = -table_sign[n.turn];
      } else if(n.hand2 < n.hide) {
        reward2 = table_sign[n.turn];
      }
    } else {
      if(n.barrier2 == false) {
        if(n.hand2 == 8) {
          if(g == MRND_DS) end_points++;
          reward1 = table_sign[n.turn];
        } else {
          if(g == MRND_DS) rand_points++;
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            work_do_action w2 = w0;
            double p2 = n.deck[i - 1];
            n.do_action(6, i, w2);
            reward1 += wizard(n) * p1 * p2;
            n.undo_action(6, i, w2);
          }
        }
      } else {
        if(g == MCFR) {
          work_do_action ww = w0;
          n.do_action(6, 0, ww);
          reward1 = wizard(n);
          n.undo_action(6, 0, ww);
        } else if(g == MCFR_EXP_REWARD) {
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            work_do_action ww = w0;
            double p2 = n.deck[i - 1];
            n.do_action(6, i, ww);
            n.do_action(4, i, ww);
            reward1 += draw(n) * p1 * p2;
            n.undo_action(4, i, ww);
            n.undo_action(6, i, ww);
          }
        } else if(g == MRND_DS) {
          rand_points++;
          for(int i = 1; i < 9; i++) {
            if(n.deck[i - 1] == 0) {
              continue;
            }
            work_do_action ds_ww = w0;
            n.do_action(6, i, ds_ww);
            n.do_action(4, i, ds_ww);
            draw(n);
            n.undo_action(4, i, ds_ww);
            n.undo_action(6, i, ds_ww);
          }
        }
      }
      if(n.hand1[0] == 8) {
        if(g == MRND_DS) end_points++;
        reward2 = -table_sign[n.turn];
      } else {
        if(g == MRND_DS) rand_points++;
        for(int i = 1; i < 9; i++) {
          if(n.deck[i - 1] == 0) {
            continue;
          }
          work_do_action w3 = w0;
          double p2 = n.deck[i - 1];
          n.do_action(7, i, w3);
          reward2 += wizard_self(n) * p1 * p2;
          n.undo_action(7, i, w3);
        }
      }
    }
    reward = reward1 * p + reward2 * (1 - p);
    if(g == MCFR) {
      w0.infset_it->second.set_regret(0, w0.infset_it->second.get_regret(0) + n.npi[1 - n.turn][n.depth] * n.npi[2][n.depth] * (reward1 - reward));
      w0.infset_it->second.set_regret(1, w0.infset_it->second.get_regret(1) + n.npi[1 - n.turn][n.depth] * n.npi[2][n.depth] * (reward2 - reward));
    }
    return reward;
  } break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    if(g == MRND_DS) end_points++;
    return -table_sign[n.turn];
  }
  if(n.dsum() == 0) {
    if(g == MRND_DS) {
      end_points++;
      return 0.0;
    }
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return -table_sign[n.turn];
    } else {
      return 0.0;
    }
  }

  if(g == MRND_DS) rand_points++;
  reward = 0.0;
  double p1 = n.dsum_rec();
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += draw(n) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}

double wizard(node &n) {
  if(n.dsum() == 0) {
    if(g == MRND_DS) {
      end_points++;
      return 0.0;
    }
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return -table_sign[n.turn];
    } else {
      return 0.0;
    }
  }

  double reward = 0.0;
  double p1 = n.dsum_rec();
  if(g == MRND_DS) {
    rand_points++;
  }
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action w;
    double p2 = n.deck[i - 1];
    n.do_action(4, i, w);
    reward += draw(n) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}
double wizard_self(node &n) {
  if(n.dsum() == 0) {
    if(g == MRND_DS) {
      end_points++;
    }
    if(n.hand1[0] > n.hand2) {
      return table_sign[n.turn];
    } else if(n.hand1[0] < n.hand2) {
      return -table_sign[n.turn];
    } else {
      return 0.0;
    }
  }
  if(g == MRND_DS) {
    rand_points++;
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
    reward += draw(n) * p1 * p2;
    n.undo_action(4, i, w);
  }
  return reward;
}
