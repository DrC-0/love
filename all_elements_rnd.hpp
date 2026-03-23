#include "rnd_action.hpp"

void all_put_hide_card(string &h, unsigned long int head, node &n);
void all_draw_p1_init(string &h, unsigned long int head, node &n);
void all_draw_p2_init(string &h, unsigned long int head, node &n);
void all_draw(string &h, unsigned long int head, node &n);
void all_play(string &h, unsigned long int head, node &n, int c);
void all_wizard(string &h, unsigned long int head, node &n);
void all_wizard_self(string &h, unsigned long int head, node &n);
void all_soldior(string &h, unsigned long int head, node &n);

void all_put_hide_card(string &h, unsigned long int head, node &n){
  string action = rph.get_action((unsigned char)h[head]);
  int c2a = char_to_action(action[0]);
  int c2a1 = c2a / 10; int c2a2 = c2a % 10;
  if(c2a1 == 1){
    if(n.deck[c2a2] == 0) { return; }
    work_do_action w;
    head++;
    n.do_action(2, c2a2 + 1, w);
    all_draw_p1_init(h, head, n);
    n.undo_action(2, c2a2 + 1, w);
    head--;
  } else {
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(2, i, w);
      all_draw_p1_init(h, head, n);
      n.undo_action(2, i, w);
    }
  }
  return;
}

void all_draw_p1_init(string &h, unsigned long int head, node &n){
  string action = rph.get_action((unsigned char)h[head]);
  int c2a = char_to_action(action[0]);
  int c2a1 = c2a / 10; int c2a2 = c2a % 10;
  if(c2a1 == 2){
    if(n.deck[c2a2] == 0) { return; }
    work_do_action w;
    head++;
    n.do_action(3, c2a2 + 1, w);
    all_draw_p2_init(h, head, n);
    n.undo_action(3, c2a2 + 1, w);
    head--;
  } else {  
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(3, i, w);
      all_draw_p2_init(h, head, n);
      n.undo_action(3, i, w);
    }
  }
  return;
}

void all_draw_p2_init(string &h, unsigned long int head, node &n){
  string action = rph.get_action((unsigned char)h[head]);
  int c2a = char_to_action(action[0]);
  int c2a1 = c2a / 10; int c2a2 = c2a % 10;
  if(c2a1 == 3){
    if(n.deck[c2a2] == 0) { return; }
    work_do_action w;
    head++;
    n.do_action(4, c2a2 + 1, w);
    all_draw(h, head, n);
    n.undo_action(4, c2a2 + 1, w);
    head--;
  } else {  
    for(int i = 1; i < 9; i++){
      if(n.deck[i-1] == 0) { continue; }
      work_do_action w;
      n.do_action(4, i, w);
      all_draw(h, head, n);
      n.undo_action(4, i, w);
    }
  }
  return;
}

void all_draw(string &h, unsigned long int head, node &n){
    if((n.hand1[0] == 7 || n.hand1[1] == 7) && n.hand1[0] + n.hand1[1] >= 12) {
      return;
    }
    //必勝判定
    if(use_good_move){
      if(n.open2 > 1 && n.barrier2 == false){
        if(n.hand1[0] == 1 || n.hand1[1] == 1){
          return;
        }
      }
      if(n.open2 > 0 && n.barrier2 == false){
        if(n.hand1[0] == 3 && n.hand1[1] > n.open2){
          return;
        }
        if(n.hand1[1] == 3 && n.hand1[0] > n.open2){
          return;
        }
      }
      if(n.open2 == 8 && n.barrier2 == false){
        if(n.hand1[0] == 5 || n.hand1[1] == 5){
          return;
        } 
      }
    }

    if(head > h.size()){
      return;
    }

    work_do_action w;
    //必敗判定
    if(nuse_bad_move){
      if(n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false){
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        all_play(h, head, n, card);
        n.undo_action(5, card, w);
        return;
      } else if(n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false){
        int card = n.hand1[0];
        n.do_action(5, 0, w);
        all_play(h, head, n, card);
        n.undo_action(5, card, w);
        return;
      } else if(n.hand1[0] == 8) {
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        all_play(h, head, n, card);
        n.undo_action(5, card, w);
        return;
      } else if(n.hand1[1] == 8){
        int card = n.hand1[0];
        n.do_action(5, 0, w);
        all_play(h, head, n, card);
        n.undo_action(5, card, w);
        return;
      }
    }
    //手札が同じカード2枚のとき
    if(use_same_move){
      if(n.hand1[0] == n.hand1[1]){
        int card = n.hand1[1];
        n.do_action(5, 1, w);
        all_play(h, head, n, card);
        n.undo_action(5, card, w);
        return;
      }
    }

    if(n.rnd_his_p[n.turn].get_hash_value() == h){
      all_history.emplace_back(n.rnd_his.get_hash_value());
      return;
    }

    int c1, c2;
    c1 = n.hand1[0]; c2 = n.hand1[1];
    if(n.turn == br_player){
      w.infset_it = table_infset.begin();
    } else {
      w.infset_it = table_infset.find(n.rnd_his_p[n.turn].get_hash_value());
    }
    work_do_action w2 = w;
    if(head >= h.size()) return;
    int hash = (unsigned char)h[head];
     if(hash > RND_MAX_HASH_VALUE) {
      cout << "all_draw" << endl;
      n.print_history();
      terminate();
     }
    string action = rph.get_action((unsigned char)h[head]);
    int c2a = char_to_action(action[0]);
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    if(c2a1 == 4){
      if(!use_same_move && n.hand1[0] == n.hand1[1] && c2a2 + 1 == n.hand1[1]){
        n.do_action(5, 0, w);
        all_play(h, head, n, c1);
        n.undo_action(5, c1, w);
        n.do_action(5, 1, w2);
        all_play(h, head, n, c2);
        n.undo_action(5, c2, w2);
      } else if(c2a2 + 1 == n.hand1[1]){
        n.do_action(5, 1, w2);
        all_play(h, head, n, c2);
        n.undo_action(5, c2, w2);
      } else if(c2a2 + 1 == n.hand1[0]){
        n.do_action(5, 0, w);
        all_play(h, head, n, c1);
        n.undo_action(5, c1, w);
      } else {
        return;
      }
    }
    return;
}

void all_play(string &h, unsigned long int head, node &n, int c){
    double prob_win = 0.0;
    switch(c){
        case 1:
          if(n.open2 > 1 && n.barrier2 == false) {
            return;
          }
          if(n.hand2 > 1 && n.barrier2 == false){
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
          }
          break;
        case 2:
          if(n.barrier2 == false){
            string action = rph.get_action((unsigned char)h[head]);
            int c2t = char_to_twonum(action[1]);
            if((c2t % 10) + 1 != n.hand2) return;
          }
          break;
        case 3:
          if(n.hand1[0] > n.hand2 && n.barrier2 == false) {
            return;
          } else if (n.hand1[0] < n.hand2 && n.barrier2 == false){
            return;
          } else if(n.hand1[0] == n.hand2 && n.barrier2 == false){
            string action = rph.get_action((unsigned char)h[head]);
            int c2t = char_to_twonum(action[1]);
            if((c2t / 10) + 1 != n.hand1[0] || (c2t % 10) + 1 != n.hand2) return;
          }
          break;
        case 4:
          break;
        case 5: {
            if(n.rnd_his_p[n.turn].get_hash_value() == h){
              all_history.emplace_back(n.rnd_his.get_hash_value());
              return;
            } else if(head >= h.size()){
              return;
            }
            if(n.dsum() == 0){
              return;
            }
            work_do_action all_w0;
            if(n.turn == br_player){
              all_w0.infset_it = table_infset.begin();
            } else {
              all_w0.infset_it = table_infset.find(n.rnd_his_p[n.turn].get_hash_value());
            }
            string action = rph.get_action((unsigned char)h[head]);
            int c2w = char_to_wizard(action[1]);
            int to = c2w / 100;
            int trash = (c2w / 10) % 10;
            int draw = c2w % 10;
            action = rph.get_action((unsigned char)h[0]);
            int c2a0 = char_to_action(action[0]);
            int c2a01 = c2a0 / 10;
            if((c2a01 == 1 && to == 1 && n.turn == 0) || (c2a01 == 2 && to == 0 && n.turn == 1)){  
              if(n.barrier2 == false){
                if(n.hand2 != trash + 1) return;
                if(n.hand2 == 8) return;
                for(int i = 1; i < 9; i++){
                  if(n.deck[i-1] == 0) { continue; }
                  work_do_action all_w2 = all_w0; head++;
                  n.do_action(6, i, all_w2);
                  all_wizard(h, head, n);
                  n.undo_action(6, i, all_w2); head--;
                }
              } else {
                work_do_action all_ww = all_w0; head++;
                n.do_action(6, 0, all_ww);
                all_wizard(h, head, n);
                n.undo_action(6, 0, all_ww); head--;
              }
            } else if((c2a01 == 1 && to == 0 && n.turn == 1) || (c2a01 == 2 && to == 1 && n.turn == 0)){
              if(n.barrier2 == false){
                if(n.hand2 != trash + 1) return;
                if(n.deck[draw] == 0) return;
                if(n.hand2 == 8) return;
                work_do_action all_w3 = all_w0; head++;
                n.do_action(6, draw + 1, all_w3);
                all_wizard_self(h, head, n);
                n.undo_action(6, draw + 1, all_w3); head--;
              } else {
                head++;
                string action = rph.get_action((unsigned char)h[head]);
                int c2a = char_to_action(action[0]);
                int ctoa2_a = c2a / 10;
                int ctoa2_b = c2a % 10;
                if(ctoa2_a == 3){
                  if(n.deck[ctoa2_b] == 0) { return; }
                  work_do_action all_ww = all_w0; head++;
                  n.do_action(6, 1, all_ww);
                  n.do_action(4, ctoa2_b + 1, all_ww);
                  all_draw(h, head, n);
                  n.undo_action(4, ctoa2_b + 1, all_ww);
                  n.undo_action(6, 1, all_ww); head--;
                } else {
                  work_do_action all_ww = all_w0; head++;
                  n.do_action(6, 0, all_ww);
                  all_wizard(h, head, n);
                  n.undo_action(6, 0, all_ww); head--;
                }
              }
            } else if((c2a01 == 1 && to == 1 && n.turn == 1) || (c2a01 == 2 && to == 0 && n.turn == 0)){
              if(n.hand1[0] != trash + 1) return;
              if(n.hand1[0] == 8) return;
              for(int i = 1; i < 9; i++){
                if(n.deck[i-1] == 0) { continue; }
                work_do_action all_w2 = all_w0; head++;
                n.do_action(7, i, all_w2);
                all_wizard_self(h, head, n);
                n.undo_action(7, i, all_w2); head--;
              }
            } else {
              if(n.hand1[0] != trash + 1) return;
              if(n.deck[draw] == 0) return;
              if(n.hand1[0] == 8) return;
              work_do_action all_w3 = all_w0; head++;
              n.do_action(7, draw + 1, all_w3);
              all_wizard_self(h, head, n);
              n.undo_action(7, draw + 1, all_w3); head--;
            }
            return;
          }
          break;
        case 6:
          if(n.barrier2 == false){
            string action = rph.get_action((unsigned char)h[head]);
            int c2t = char_to_twonum(action[1]);
            int give = c2t / 10;
            int take = c2t % 10;
            string p = rph.get_action((unsigned char)h[0]);
            int c2ap = char_to_action(p[0]);
            if((c2ap / 10) - 1 == n.turn && (give + 1 != n.hand2 || take + 1 != n.hand1[0])) return;
            if((c2ap / 10) - 1 != n.turn && (give + 1 != n.hand1[0] || take + 1 != n.hand2)) return;
          }
          break;
        case 7:
          break;
        case 8:
          return;
    }
    if(n.dsum() == 0){
      return;
    }

    head++;
    if(head >= h.size()) return;
    string next = rph.get_action((unsigned char)h[head]);
    int c2an = char_to_action(next[0]);
    int naction = c2an / 10;
    int ncard = c2an % 10;
    if(naction == 3){
      if(n.deck[ncard] == 0) { return; }
      work_do_action w;
      w.randsol_wprob = prob_win;
      head++;
      n.do_action(4, ncard + 1, w);
      all_draw(h, head, n);
      n.undo_action(4, ncard + 1, w);
      head--;
    } else {
      for(int i = 1; i < 9; i++){
        if(n.deck[i-1] == 0) { continue; }
        work_do_action w;
        w.randsol_wprob = prob_win;
        n.do_action(4, i, w);
        all_draw(h, head, n);
        n.undo_action(4, i, w);
      }
    }
    head--;
    return;
}

void all_wizard(string &h, unsigned long int head, node &n){
    if(n.dsum() == 0){
      return;
    }
    if(head >= h.size()) return;
    string action = rph.get_action((unsigned char)h[head]);
    int c2a = char_to_action(action[0]);
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    if(c2a1 == 3){
      if(n.deck[c2a2] == 0) { return; }
      work_do_action w; head++;
      n.do_action(4, c2a2 + 1, w);
      all_draw(h, head, n);
      n.undo_action(4, c2a2 + 1, w); head--;
    } else {
      for(int i = 1; i < 9; i++){
        if(n.deck[i-1] == 0) { continue; }
        work_do_action w;
        n.do_action(4, i, w);
        all_draw(h, head, n);
        n.undo_action(4, i, w);
      }
    }
    return;
}
void all_wizard_self(string &h, unsigned long int head, node &n){
    if(n.dsum() == 0){
      return;
    }
    if(head >= h.size()) return;
    string action = rph.get_action((unsigned char)h[head]);
    int c2a = char_to_action(action[0]);
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    if(c2a1 == 3){
      if(n.deck[c2a2] == 0) { return; }
      work_do_action w; head++;
      n.do_action(4, c2a2 + 1, w);
      all_draw(h, head, n);
      n.undo_action(4, c2a2 + 1, w); head--;
    } else {
      for(int i = 1; i < 9; i++){
        if(n.deck[i-1] == 0) { continue; }
        work_do_action w;
        n.do_action(4, i, w);
        all_draw(h, head, n);
        n.undo_action(4, i, w);
      }
    }
    return;
}
void all_soldior(string &h, unsigned long int head, node &n){
    if(n.dsum() == 0){
      return;
    }
    if(head >= h.size()) return;
    string action = rph.get_action((unsigned char)h[head]);
    int c2a = char_to_action(action[0]);
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    if(c2a1 == 3){
      if(n.deck[c2a2] == 0) { return; }
      work_do_action w; head++;
      n.do_action(4, c2a2 + 1, w);
      all_draw(h, head, n);
      n.undo_action(4, c2a2 + 1, w); head--;
    } else {
      for(int i = 1; i < 9; i++){
        if(n.deck[i-1] == 0) { continue; }
        work_do_action w;
        n.do_action(4, i, w);
        all_draw(h, head, n);
        n.undo_action(4, i, w);
      }
    }
    return;
}