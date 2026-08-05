#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<fstream>
#include<map>
#include<ctime>
#include<cstdlib>
#include<random>
#include<vector>
#include<string>
#include<numeric>
#include<cassert>
#include<functional>

using namespace std;

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "org_action.hpp"
#include "loveletter.hpp"

extern std::map<std::string, infset> table_infset;
//extern std::map<size_t, infset> table_infset;
extern int cfr_switch;
extern bool br_switch;
extern int br_player;
extern bool org_switch;

static Rnd_Perfect_Hash rph;
static Org_Perfect_Hash oph;

char action_to_char(int action, int card){
  char c = (1 << 7) | (action << 3) | card;
  return c;
}
int char_to_action(char c){
  int c2a1 = (c >> 3) & 7;
  int c2a2 = c & 7;
  return c2a1 * 10 + c2a2;
}
char wizard_to_char(int to, int trash, int draw){
  char c = (1 << 7) | (to << 6) | (trash << 3) | draw;
  return c;
}
int char_to_wizard(char c){
  int c2w1 = (c >> 6) & 1;
  int c2w2 = (c >> 3) & 7;
  int c2w3 = c & 7;
  return c2w1 * 100 + c2w2 * 10 + c2w3;
}
char twonum_to_char(int card1, int card2){
  char c = (1 << 7) | (card1 << 3) | card2;
  return c;
}
int char_to_twonum(char c){
  int c2t1 = (c >> 3) & 7;
  int c2t2 = c & 7;
  return c2t1 * 10 + c2t2;
}

node::node(const int input_open[3]) : depth (0), hand1 {0, 0}, hand2 (0), open1 (0), open2 (0), deck {n1, n2, n3, n4, n5, n6, n7, n8}, open {0, 0, 0}, hide (0), turn (0), count_turn (0), barrier1 (false), barrier2 (false) {
  assert(0 < input_open[0] && input_open[0] < 9);
  assert(0 < input_open[1] && input_open[1] < 9);
  assert(0 < input_open[2] && input_open[2] < 9);
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < 32; j++){
      npi[i][j] = 1.0;
    }
    open[i] = input_open[i];
    deck[open[i]-1] -= 1;
  }
  for(int k = 0; k < 8; k++){
    assert(deck[k] > -1);
  }
}

node::node(const std::string &h, bool rnd, const int input_open[3]) : depth (0), hand1 {0, 0}, hand2 (0), open1 (0), open2 (0), deck {n1, n2, n3, n4, n5, n6, n7, n8}, open {0, 0, 0}, hide (0), turn (0), count_turn (0), barrier1 (false), barrier2 (false) {
  assert(0 < input_open[0] && input_open[0] < 9);
  assert(0 < input_open[1] && input_open[1] < 9);
  assert(0 < input_open[2] && input_open[2] < 9);
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < 32; j++){
      npi[i][j] = 1.0;
    }
    open[i] = input_open[i];
    deck[open[i]-1] -= 1;
  }
  for(int k = 0; k < 8; k++){
    assert(deck[k] > -1);
  }
  long unsigned int head = 0;
  double prob_win = 0.0;
  while(head < h.size()){
    string key; string key2;
    string action;
    if(rnd){
      action = rph.get_action((unsigned char)h[head]);
    } else {
      action = oph.get_action((unsigned char)h[head]);
    }
    int c2a = char_to_action(action[0]); head++;
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    switch(c2a1){
      case 0:
        key = {action_to_char(c2a1, c2a2)};
        rnd_his.push(key.c_str(), 1);
        org_his.push(key.c_str(), 1);
        npi[2][depth+1] = npi[2][depth] * deck[c2a2] * dsum_rec();
        hide = c2a2 + 1;
        deck[c2a2] -= 1;
        depth++;
        break;
      case 1:
        key = {action_to_char(c2a1, c2a2)};
        rnd_his.push(key.c_str(), 1); rnd_his_p[0].push(key.c_str(), 1);
        org_his.push(key.c_str(), 1); org_his_p[0].push(key.c_str(), 1);
        npi[2][depth+1] = npi[2][depth] * deck[c2a2] * dsum_rec();
        hand1[0] = c2a2 + 1;
        deck[c2a2] -= 1;
        depth++;
        break;
      case 2:
        key = {action_to_char(c2a1, c2a2)};
        rnd_his.push(key.c_str(), 1); rnd_his_p[1].push(key.c_str(), 1);
        org_his.push(key.c_str(), 1); org_his_p[1].push(key.c_str(), 1);
        npi[2][depth+1] = npi[2][depth] * deck[c2a2] * dsum_rec();
        hand2 = c2a2 + 1;
        deck[c2a2] -= 1;
        depth++;
        break;
      case 3:
        if(count_turn != 0){
          turn = 1 - turn;
          std::swap(hand1[0], hand2);
          std::swap(open1, open2);
          barrier2 = false;
          std::swap(barrier1, barrier2);
        }
        key = {action_to_char(c2a1, c2a2)};
        rnd_his.push(key.c_str(), 1);  rnd_his_p[turn].push(key.c_str(), 1);
        org_his.push(key.c_str(), 1);  org_his_p[turn].push(key.c_str(), 1);
        npi[0][depth+1] = npi[0][depth];
        npi[1][depth+1] = npi[1][depth];
        npi[2][depth+1] = npi[2][depth] * (1.0 - prob_win) * deck[c2a2] * dsum_rec();
        prob_win = 0.0;
        hand1[1] = c2a2 + 1;
        deck[c2a2] -= 1;
        count_turn++;
        depth++;
        break;
      case 4:
        if(c2a2 + 1 == open1) open1 = 0;
        if((use_same_move && hand1[0] == hand1[1]) || (nuse_bad_move && ((hand1[0] == 3 && hand1[1] < open2 && barrier2 == false) || hand1[0] == 8))){
          hand1[1] = 0;
          npi[0][depth+1] = npi[0][depth];
          npi[1][depth+1] = npi[1][depth];
          npi[2][depth+1] = npi[2][depth];
        } else if(nuse_bad_move && ((hand1[0] < open2 && hand1[1] == 3 && barrier2 == false) || hand1[1] == 8)){
          hand1[0] = hand1[1]; hand1[1] = 0;
          npi[0][depth+1] = npi[0][depth];
          npi[1][depth+1] = npi[1][depth];
          npi[2][depth+1] = npi[2][depth];
        } else if(hand1[0] == c2a2 + 1){
          hand1[0] = hand1[1]; hand1[1] = 0;
          double p;
          if(!br_switch){
            infset &x = table_infset[rnd_his_p[turn].get_hash_value()];
            p = x.get_prob_action();
#ifdef CFR
            if(cfr_switch){
              assert(x.get_sum_i(1) != 0);
              p = x.get_sum_i(0) / x.get_sum_i(1);
            }
#endif
#ifdef CFR_SELF_PLAY
            if(cfr_switch){
              assert(x.infset_it->second.get_sum_i(1) != 0);
              p = (double)x.get_sum_i(0) / x.get_sum_i(1);
            }
#endif
          } else {
            if(turn == br_player){
              p = 1;
            } else {
              infset &x = table_infset[rnd_his_p[turn].get_hash_value()];
              p = x.get_prob_action();
#ifdef CFR
              if(cfr_switch){
                assert(x.get_sum_i(1) != 0);
                p = x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
#ifdef CFR_SELF_PLAY
              if(cfr_switch){
                assert(x.infset_it->second.get_sum_i(1) != 0);
                p = (double)x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
            }
          }
          npi[turn][depth+1] = npi[turn][depth] * p;
          npi[1-turn][depth+1] = npi[1-turn][depth];
          npi[2][depth+1] = npi[2][depth];
        } else {
          hand1[1] = 0;
          double p;
          if(!br_switch){
            infset &x = table_infset[rnd_his_p[turn].get_hash_value()];
            p = x.get_prob_action();
#ifdef CFR
            if(cfr_switch){
              assert(x.get_sum_i(1) != 0);
              p = x.get_sum_i(0) / x.get_sum_i(1);
            }
#endif
#ifdef CFR_SELF_PLAY
            if(cfr_switch){
              assert(x.infset_it->second.get_sum_i(1) != 0);
              p = (double)x.get_sum_i(0) / x.get_sum_i(1);
            }
#endif
          } else {
            if(turn == br_player){
              p = 0;
            } else {
              infset &x = table_infset[rnd_his_p[turn].get_hash_value()];
              p = x.get_prob_action();
#ifdef CFR
              if(cfr_switch){
                assert(x.get_sum_i(1) != 0);
                p = x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
#ifdef CFR_SELF_PLAY
              if(cfr_switch){
                assert(x.infset_it->second.get_sum_i(1) != 0);
                p = (double)x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
            }
          }
          npi[turn][depth+1] = npi[turn][depth] * (1 - p);
          npi[1-turn][depth+1] = npi[1-turn][depth];
          npi[2][depth+1] = npi[2][depth];
        }
        depth++;
        key = {action_to_char(c2a1, c2a2)};
        rnd_his.push(key.c_str(), 1); rnd_his_p[0].push(key.c_str(), 1); rnd_his_p[1].push(key.c_str(), 1);
        org_his.push(key.c_str(), 1); org_his_p[0].push(key.c_str(), 1); org_his_p[1].push(key.c_str(), 1);
        if(c2a2 + 1 == 1 && barrier2 == false && action.size() > 1){ //兵士宣言
          int c2t = char_to_twonum(action[1]);
          int c2t2 = c2t % 10;
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          key = {action_to_char(4, 0), twonum_to_char(0, c2t2)};
          org_his.push(key.c_str(), 2); org_his_p[0].push(key.c_str(), 2); org_his_p[1].push(key.c_str(), 2);
          npi[0][depth+1] = npi[0][depth];
          npi[1][depth+1] = npi[1][depth];
          npi[2][depth+1] = npi[2][depth];
          depth++;
        } else if(c2a2 + 1 == 1 && hand2 > 1 && barrier2 == false){
          if(!br_switch){
            double exist_card[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            for(int i = 0; i < 8; i++) exist_card[i] += deck[i];
            exist_card[hand2 - 1]++;
            exist_card[hide - 1]++;
            //exist_card[1] = exist_card[1] / 2.0;
            //exist_card[7] = exist_card[7] * 8.0;
            double sum_exist_card = exist_card[1] + exist_card[2] + exist_card[3] + exist_card[4] + exist_card[5] + exist_card[6] + exist_card[7];
            prob_win = exist_card[hand2 - 1] / sum_exist_card;
            if(exist_card[7] > 0){
              prob_win = 0.0;
              if(hand2 == 8) prob_win = 1.0;
            }
            /*
            if(hide != 1){
              prob_win = deck[1] + deck[2] + deck[3] + deck[4] + deck[5] + deck[6] + deck[7] + 2;
              if(hide != hand2){
                prob_win = (1 + deck[hand2 - 1]) / prob_win;
              } else {
                prob_win = (1 + 1) / prob_win;
              }
            } else {
              prob_win = deck[1] + deck[2] + deck[3] + deck[4] + deck[5] + deck[6] + deck[7] + 1;
              prob_win = (1 + deck[hand2 - 1]) / prob_win;
            }
            */
          } else {
            if(turn == br_player){
              prob_win = 0.0;
            } else {
              double exist_card[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
              for(int i = 0; i < 8; i++) exist_card[i] += deck[i];
              exist_card[hand2 - 1]++;
              exist_card[hide - 1]++;
              //exist_card[1] = exist_card[1] / 2.0;
              //exist_card[7] = exist_card[7] * 8.0;
              double sum_exist_card = exist_card[1] + exist_card[2] + exist_card[3] + exist_card[4] + exist_card[5] + exist_card[6] + exist_card[7];
              prob_win = exist_card[hand2 - 1] / sum_exist_card;
              if(exist_card[7] > 0){
                prob_win = 0.0;
                if(hand2 == 8) prob_win = 1.0;
              }
              /*
              if(hide != 1){
                prob_win = deck[1] + deck[2] + deck[3] + deck[4] + deck[5] + deck[6] + deck[7] + 2;
                if(hide != hand2){
                  prob_win = (1 + deck[hand2 - 1]) / prob_win;
                } else {
                  prob_win = (1 + 1) / prob_win;
                }
              } else {
                prob_win = deck[1] + deck[2] + deck[3] + deck[4] + deck[5] + deck[6] + deck[7] + 1;
                prob_win = (1 + deck[hand2 - 1]) / prob_win;
              }
              */
            }
          }
        }else if(c2a2 + 1 == 2 && barrier2 == false){
          int c2t = char_to_twonum(action[1]);
          int c2t2 = c2t % 10;
          open2 = c2t2 + 1;
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          key = {action_to_char(4, 1), twonum_to_char(0, c2t2)};
          rnd_his.push(key.c_str(), 2); rnd_his_p[0].push(key.c_str(), 2); rnd_his_p[1].push(key.c_str(), 2);
          org_his.push(key.c_str(), 2); org_his_p[0].push(key.c_str(), 2); org_his_p[1].push(key.c_str(), 2);
        } else if(c2a2 + 1 == 3 && barrier2 == false && hand1[0] == hand2){
          open1 = hand1[0]; open2 = hand2;
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          key = {action_to_char(4, 2), twonum_to_char(hand1[0] - 1, hand2 - 1)};
          rnd_his.push(key.c_str(), 2); rnd_his_p[0].push(key.c_str(), 2); rnd_his_p[1].push(key.c_str(), 2);
          org_his.push(key.c_str(), 2); org_his_p[0].push(key.c_str(), 2); org_his_p[1].push(key.c_str(), 2);
        } else if(c2a2 + 1 == 4){
          barrier1 = true;
        } else if(c2a2 + 1 == 5){
          count_turn++;
          if(action.size() > 1){
            double p = 0.0;
            if(!br_switch){
              infset &x = table_infset[rnd_his_p[turn].get_hash_value()];
              p = x.get_prob_action();
#ifdef CFR
              if(cfr_switch){
                assert(x.get_sum_i(1) != 0);
                p = x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
#ifdef CFR_SELF_PLAY
              if(cfr_switch){
                assert(x.infset_it->second.get_sum_i(1) != 0);
                p = (double)x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
            } else {
              if(turn != br_player){
                infset &x = table_infset[rnd_his_p[turn].get_hash_value()];
                p = x.get_prob_action();
#ifdef CFR
                if(cfr_switch){
                  assert(x.get_sum_i(1) != 0);
                  p = x.get_sum_i(0) / x.get_sum_i(1);
                }
#endif
#ifdef CFR_SELF_PLAY
              if(cfr_switch){
                assert(x.infset_it->second.get_sum_i(1) != 0);
                p = (double)x.get_sum_i(0) / x.get_sum_i(1);
              }
#endif
              }
            }
            int c2w = char_to_wizard(action[1]);
            int c2w1 = c2w / 100; int c2w2 = (c2w / 10) % 10; int c2w3 = c2w % 10;
            int to = c2w1;
            int trash = c2w2;
            int draw = c2w3;
            if(to == turn || barrier2 == false){
              if(to == turn){
                open1 = 0;
                hand1[0] = draw + 1;
              } else {
                open2 = 0;
                hand2 = draw + 1;
              }
              rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
              org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
              key = {action_to_char(4, 4), wizard_to_char(to, trash, draw)}; key2 = {action_to_char(4, 4), wizard_to_char(to, trash, 0)};
              rnd_his.push(key.c_str(), 2); rnd_his_p[to].push(key.c_str(), 2); rnd_his_p[1-to].push(key2.c_str(), 2);
              org_his.push(key.c_str(), 2); org_his_p[to].push(key.c_str(), 2); org_his_p[1-to].push(key2.c_str(), 2);
              npi[2][depth+1] = npi[2][depth] * deck[draw] * dsum_rec();
              deck[draw] -= 1;
            } else {
              rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
              org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
              key = {action_to_char(4, 4), wizard_to_char(to, 7, 7)};
              rnd_his.push(key.c_str(), 2); rnd_his_p[0].push(key.c_str(), 2); rnd_his_p[1].push(key.c_str(), 2);
              org_his.push(key.c_str(), 2); org_his_p[0].push(key.c_str(), 2); org_his_p[1].push(key.c_str(), 2);
              npi[2][depth+1] = npi[2][depth];
            }
            if(br_switch && turn == br_player){
              npi[turn][depth+1] = npi[turn][depth];
            } else if(to == turn){
              npi[turn][depth+1] = npi[turn][depth] * (1 - p);
            } else {
              npi[turn][depth+1] = npi[turn][depth] * p;
            }
            npi[1-turn][depth+1] = npi[1-turn][depth];
            depth++;
          }
        } else if(c2a2 + 1 == 6 && barrier2 == false){
          int c2t = char_to_twonum(action[1]);
          int c2t1 = c2t / 10; int c2t2 = c2t % 10;
          int card1 = c2t1 + 1;
          int card2 = c2t2 + 1;
          hand1[0] = card2; open1 = card2;
          hand2 = card1; open2 = card1;
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          key = {action_to_char(4, 5), twonum_to_char(card1 - 1, card2 - 1)}; key2 = {action_to_char(4, 5), twonum_to_char(card2 - 1, card1 - 1)};
          rnd_his.push(key.c_str(), 2); rnd_his_p[turn].push(key.c_str(), 2); rnd_his_p[1-turn].push(key2.c_str(), 2);
          org_his.push(key.c_str(), 2); org_his_p[turn].push(key.c_str(), 2); org_his_p[1-turn].push(key2.c_str(), 2);
        }
        break;
    }
  }
}

bool node::operator == (const node &n)const noexcept{
  if(this == &n) return true;
  if(depth != n.depth){
    cout << "depth valid " << depth << " " << n.depth << endl;
    return false;
  }
  if(hand1[0] != n.hand1[0]){
    cout << "hand1[0] valid " << hand1[0] << " " << n.hand1[0] << endl;
    return false;
  }
  if(hand1[1] != n.hand1[1]){
    cout << "hand1[1] valid " << hand1[1] << " " << n.hand1[1] << endl;
    return false;
  }
  if(hand2 != n.hand2){
    cout << "hand2 valid " << hand2 << " " << n.hand2 << endl;
    return false;
  }
  if(open1 != n.open1){
    cout << "open1 valid " << open1 << " " << n.open1 << endl;
    return false;
  }
  if(open2 != n.open2){
    cout << "open2 valid " << open2 << " " << n.open2 << endl;
    return false;
  }
  for(int i = 0; i < 8; i++){
    if(deck[i] != n.deck[i]){
      cout << "deck[" << i << "] valid " << deck[i] << " " << n.deck[i] << endl;
      return false;
    }
  }
  for(int i = 0; i < 3; i++){
    if(open[i] != n.open[i]){
      cout << "open[" << i << "] valid " << open[i] << " " << n.open[i] << endl;
      return false;
    }
  }
  if(hide != n.hide){
    cout << "hide valid " << hide << " " << n.hide << endl;
    return false;
  }
  if(turn != n.turn){
    cout << "turn valid " << turn << " " << n.turn << endl;
    return false;
  }
  if(count_turn != n.count_turn){
    cout << "count_turn valid " << count_turn << " " << n.count_turn << endl;
    return false;
  }
  if(barrier1 != n.barrier1){
    cout << "barrier1 valid " << barrier1 << " " << n.barrier1 << endl;
    return false;
  }
  if(barrier2 != n.barrier2){
    cout << "barrier2 valid " << barrier2 << " " << n.barrier2 << endl;
    return false;
  }
  string history = rnd_his.get_hash_value(); string nhistory = n.rnd_his.get_hash_value();
  for(long unsigned int i = 0; i < history.size(); i++){
    if(history[i] != nhistory[i]){
      cout << "history[" << i << "] valid " << history[i] << " " << nhistory[i] << endl;
      return false;
    }
  }
  history = org_his.get_hash_value(); nhistory = n.org_his.get_hash_value();
  for(long unsigned int i = 0; i < history.size(); i++){
    if(history[i] != nhistory[i]){
      cout << "history[" << i << "] valid " << history[i] << " " << nhistory[i] << endl;
      return false;
    }
  }
  string history_private[2]; string nhistory_private[2];
  history_private[0] = rnd_his_p[0].get_hash_value(); nhistory_private[0] = n.rnd_his_p[0].get_hash_value();
  history_private[1] = rnd_his_p[1].get_hash_value(); nhistory_private[1] = n.rnd_his_p[1].get_hash_value();
  for(long unsigned int i = 0; i < history_private[0].size(); i++){
    if(history_private[0][i] != nhistory_private[0][i]){
      cout << "history_private[0] valid " << history_private[0] << " " << nhistory_private[0] << endl;
      return false;
    }
  }
  for(long unsigned int i = 0; i < history_private[1].size(); i++){
    if(history_private[1][i] != nhistory_private[1][i]){
      cout << "history_private[1] valid " << history_private[1] << " " << nhistory_private[1] << endl;
      return false;
    }
  }
  history_private[0] = org_his_p[0].get_hash_value(); nhistory_private[0] = n.org_his_p[0].get_hash_value();
  history_private[1] = org_his_p[1].get_hash_value(); nhistory_private[1] = n.org_his_p[1].get_hash_value();
  for(long unsigned int i = 0; i < history_private[0].size(); i++){
    if(history_private[0][i] != nhistory_private[0][i]){
      cout << "history_private[0] valid " << history_private[0] << " " << nhistory_private[0] << endl;
      return false;
    }
  }
  for(long unsigned int i = 0; i < history_private[1].size(); i++){
    if(history_private[1][i] != nhistory_private[1][i]){
      cout << "history_private[1] valid " << history_private[1] << " " << nhistory_private[1] << endl;
      return false;
    }
  }
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < depth + 1; j++){
      if(abs(npi[i][j] - n.npi[i][j]) > std::pow(10.0, -10.0)){
        cout << "npi[" << i << "][" << j << "] valid " << npi[i][j] << " " << n.npi[i][j] << endl;
        return false;
      }
    }
  }
  return true;
}

bool node::valid_data()const noexcept{
    //node n(rnd_his.get_hash_value(), open);
  if(org_switch){
    node n_org(org_his.get_hash_value(), false, open);
    bool same_org = operator == (n_org);
    if(!same_org) {
      cout << "n_org valid" << endl;
      cout << "history" << endl;
      print_history();
      cout << "node" << endl;
      cout << "hide : " << hide << " open : " << open[0] << " " << open[1] << " " << open[2] << endl;
      cout << "deck : " << deck[0] << " " << deck[1] << " " << deck[2] << " " << deck[3] << " " << deck[4] << " " << deck[5] << " " << deck[6] << " " << deck[7] << endl;
      cout << "depth : " << depth << endl;
      cout << "turn : " << turn << endl;
      cout << "hand1 : " << hand1[0] << " " << hand1[1] << " hand2 : " << hand2 << endl;
      cout << "open1 : " << open1 << " open2 : " << open2 << endl;
      cout << "barrier1 : " << barrier1 << " barrier2 : " << barrier2 << endl;
      cout << "----------------------------" << endl;
      n_org.print_node();
    }
    return same_org;
  } else {
    node n_rnd(rnd_his.get_hash_value(), true, open);
    bool same_rnd = operator == (n_rnd);
    if(!same_rnd) {
      cout << "n_rnd valid" << endl;
      cout << "history" << endl;
      print_history_rnd();
      cout << "node" << endl;
      cout << "hide : " << hide << " open : " << open[0] << " " << open[1] << " " << open[2] << endl;
      cout << "deck : " << deck[0] << " " << deck[1] << " " << deck[2] << " " << deck[3] << " " << deck[4] << " " << deck[5] << " " << deck[6] << " " << deck[7] << endl;
      cout << "depth : " << depth << endl;
      cout << "turn : " << turn << endl;
      cout << "hand1 : " << hand1[0] << " " << hand1[1] << " hand2 : " << hand2 << endl;
      cout << "open1 : " << open1 << " open2 : " << open2 << endl;
      cout << "barrier1 : " << barrier1 << " barrier2 : " << barrier2 << endl;
      cout << "----------------------------" << endl;
      n_rnd.print_node();
    }
    return same_rnd;
  }
}

void node::do_action(int a, int c, work_do_action &w){
  assert(0 < a && a < 9);
  assert(depth < 31);
  char addstr[3][16];
  int naddstr[3] = {};
  int card;
  switch(a) {
    case 1: //put_hide_card
      //if(0 >= c || c >= 9) print_history();
      assert(0 < c && c < 9);
      //if(deck[c-1] <= 0) print_history();
      assert(deck[c-1] > 0);
      npi[2][depth+1] = npi[2][depth] * deck[c-1] * dsum_rec();
      deck[c-1] -= 1; hide = c;
      addstr[0][naddstr[0]++] = action_to_char(0, c-1);
      break;
    case 2: //draw_p1_init
      //if(0 >= c || c >= 9) print_history();
      assert(0 < c && c < 9);
      //if(deck[c-1] <= 0) print_history();
      assert(deck[c-1] > 0);
      npi[2][depth+1] = npi[2][depth] * deck[c-1] * dsum_rec();
      deck[c-1] -= 1; hand1[0] = c;
      addstr[0][naddstr[0]++] = action_to_char(1, c-1);
      addstr[1][naddstr[1]++] = action_to_char(1, c-1);
      break;
    case 3: //draw_p2_init
      //if(0 >= c || c >= 9) print_history();
      assert(0 < c && c < 9);
      //if(deck[c-1] <= 0) print_history();
      assert(deck[c-1] > 0);
      npi[2][depth+1] = npi[2][depth] * deck[c-1] * dsum_rec();
      deck[c-1] -= 1; hand2 = c;
      addstr[0][naddstr[0]++] = action_to_char(2, c-1);
      addstr[2][naddstr[2]++] = action_to_char(2, c-1);
      break;
    case 4: //draw
      //if(0 >= c || c >= 9) print_history();
      assert(0 < c && c < 9);
      //if(deck[c-1] <= 0) print_history();
      assert(deck[c-1] > 0);
      if(count_turn != 0) {
        turn = 1 - turn;
        std::swap(hand1[0], hand2);
        std::swap(open1, open2);
        w.prev_barrier2 = barrier2;
        barrier2 = false;
        std::swap(barrier1, barrier2);
      }
      npi[0][depth+1] = npi[0][depth];
      npi[1][depth+1] = npi[1][depth];
      npi[2][depth+1] = npi[2][depth] * (1.0 - w.randsol_wprob) * deck[c-1] * dsum_rec();
      deck[c-1] -= 1; hand1[1] = c; count_turn++;
      addstr[0][naddstr[0]++] = action_to_char(3, c-1);
      addstr[1+turn][naddstr[1+turn]++] = action_to_char(3, c-1);
      break;
    case 5: //play
      assert(c == 0 || c == 1);
      card = hand1[c];
      if(hand1[c]==open1) {
        w.used_open1 = open1;
        open1 = 0;
      }
      if((use_same_move && hand1[0] == hand1[1]) || (nuse_bad_move && ((hand1[0] == 3 && hand1[1] < open2 && barrier2 == false) || hand1[0] == 8))){
        w.prev_play = 1;
        npi[0][depth+1] = npi[0][depth];
        npi[1][depth+1] = npi[1][depth];
        npi[2][depth+1] = npi[2][depth];
        hand1[1] = 0;
      } else if(nuse_bad_move && ((hand1[0] < open2 && hand1[1] == 3 && barrier2 == false) || hand1[1] == 8)){
        w.prev_play = 0;
        npi[0][depth+1] = npi[0][depth];
        npi[1][depth+1] = npi[1][depth];
        npi[2][depth+1] = npi[2][depth];
        hand1[0] = hand1[1];
        hand1[1] = 0;
      } else if(c==0){
        w.prev_play = 0;
        double p;
        if(br_switch && turn == br_player){
          p = 1.0;
        } else {
          p = w.infset_it->second.get_prob_action();
        }
#ifdef CFR
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
#ifdef CFR_SELF_PLAY
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
        npi[turn][depth+1] = npi[turn][depth] * p;
        npi[1-turn][depth+1] = npi[1-turn][depth];
        npi[2][depth+1] = npi[2][depth];
        hand1[0] = hand1[1];
        hand1[1] = 0;
      } else {
        w.prev_play = 1;
        double p;
        if(br_switch && turn == br_player){
          p = 0.0;
        } else {
          p = w.infset_it->second.get_prob_action();
        }
#ifdef CFR
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
#ifdef CFR_SELF_PLAY
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
        npi[turn][depth+1] = npi[turn][depth] * (1.0 - p);
        npi[1-turn][depth+1] = npi[1-turn][depth];
        npi[2][depth+1] = npi[2][depth];
        hand1[1] = 0;
      }
      assert(0 < card && card < 9);
      if(card == 1){
          addstr[0][naddstr[0]++] = action_to_char(4, 0);
          addstr[1][naddstr[1]++] = action_to_char(4, 0);
          addstr[2][naddstr[2]++] = action_to_char(4, 0);
      } else if(card == 2) {
          if(barrier2 == false){
            w.prev_open2 = open2;
            open2 = hand2;
            addstr[0][naddstr[0]++] = action_to_char(4, 1); addstr[0][naddstr[0]++] = twonum_to_char(0, hand2 - 1);
            addstr[1][naddstr[1]++] = action_to_char(4, 1); addstr[1][naddstr[1]++] = twonum_to_char(0, hand2 - 1);
            addstr[2][naddstr[2]++] = action_to_char(4, 1); addstr[2][naddstr[2]++] = twonum_to_char(0, hand2 - 1);
          } else {
            addstr[0][naddstr[0]++] = action_to_char(4, 1);
            addstr[1][naddstr[1]++] = action_to_char(4, 1);
            addstr[2][naddstr[2]++] = action_to_char(4, 1);
          }
      } else if(card == 3) {
          if(hand1[0] == hand2 && barrier2 == false) {
            if(hand1[0] == 3 || hand1[0] > 5) {
              print_history();
              cout << c << " " << card << " " << hand1[0] << " " << hand2 << endl;
              terminate();
            }
            w.prev_open1 = open1; w.prev_open2 = open2;
            open1 = hand1[0]; open2 = hand2;
            addstr[0][naddstr[0]++] = action_to_char(4, 2); addstr[0][naddstr[0]++] = twonum_to_char(hand1[0] - 1, hand2 - 1);
            addstr[1][naddstr[1]++] = action_to_char(4, 2); addstr[1][naddstr[1]++] = twonum_to_char(hand1[0] - 1, hand2 - 1);
            addstr[2][naddstr[2]++] = action_to_char(4, 2); addstr[2][naddstr[2]++] = twonum_to_char(hand1[0] - 1, hand2 - 1);
          } else {
            addstr[0][naddstr[0]++] = action_to_char(4, 2);
            addstr[1][naddstr[1]++] = action_to_char(4, 2);
            addstr[2][naddstr[2]++] = action_to_char(4, 2);
          }
      } else if(card == 4) {
          barrier1 = true;
          addstr[0][naddstr[0]++] = action_to_char(4, 3);
          addstr[1][naddstr[1]++] = action_to_char(4, 3);
          addstr[2][naddstr[2]++] = action_to_char(4, 3);
      } else if(card == 5) {
          addstr[0][naddstr[0]++] = action_to_char(4, 4);
          addstr[1][naddstr[1]++] = action_to_char(4, 4);
          addstr[2][naddstr[2]++] = action_to_char(4, 4);
          count_turn++;
      } else if(card == 6) {
          if(barrier2 == false){
            if(hand1[0] == 6 || hand2 == 6){
              print_history();
              terminate();
            }
            addstr[0][naddstr[0]++] = action_to_char(4, 5); addstr[0][naddstr[0]++] = twonum_to_char(hand1[0] - 1, hand2 - 1);
            addstr[1+turn][naddstr[1+turn]++] = action_to_char(4, 5); addstr[1+turn][naddstr[1+turn]++] = twonum_to_char(hand1[0] - 1, hand2 - 1);
            addstr[2-turn][naddstr[2-turn]++] = action_to_char(4, 5); addstr[2-turn][naddstr[2-turn]++] = twonum_to_char(hand2 - 1, hand1[0] - 1);
            std::swap(hand1[0], hand2);
            w.prev_open1 = open1; w.prev_open2 = open2;
            open1 = hand1[0]; open2 = hand2;
          } else {
            addstr[0][naddstr[0]++] = action_to_char(4, 5);
            addstr[1][naddstr[1]++] = action_to_char(4, 5);
            addstr[2][naddstr[2]++] = action_to_char(4, 5);
          }
      } else if(card == 7) {
          addstr[0][naddstr[0]++] = action_to_char(4, 6);
          addstr[1][naddstr[1]++] = action_to_char(4, 6);
          addstr[2][naddstr[2]++] = action_to_char(4, 6);
      } else if(card == 8) {
          addstr[0][naddstr[0]++] = action_to_char(4, 7);
          addstr[1][naddstr[1]++] = action_to_char(4, 7);
          addstr[2][naddstr[2]++] = action_to_char(4, 7);
      }
      break;
    case 6: //wizard
      if(barrier2 == false){
        //if(0 >= c || c >= 9) print_history();
        assert(0 < c && c < 9);
        //if(deck[c-1] <= 0) print_history();
        assert(deck[c-1] > 0);
        w.prev_open2 = open2;
        open2 = 0;
        double p;
        if(br_switch && turn == br_player){
          p = 1.0;
        } else {
          p = w.infset_it->second.get_prob_action();
        }
#ifdef CFR
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
#ifdef CFR_SELF_PLAY
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
        npi[turn][depth+1] = npi[turn][depth] * p;
        npi[1-turn][depth+1] = npi[1-turn][depth];
        npi[2][depth+1] = npi[2][depth] * deck[c-1] * dsum_rec();
        rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
        org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
        addstr[0][naddstr[0]++] = action_to_char(4, 4); addstr[0][naddstr[0]++] = wizard_to_char(1-turn, hand2 - 1, c - 1);
        addstr[2-turn][naddstr[2-turn]++] = action_to_char(4, 4); addstr[2-turn][naddstr[2-turn]++] = wizard_to_char(1-turn, hand2 - 1, c - 1);
        addstr[1+turn][naddstr[1+turn]++] = action_to_char(4, 4); addstr[1+turn][naddstr[1+turn]++] = wizard_to_char(1-turn, hand2 - 1, 0);
        deck[c-1] -= 1;
        w.prev_hand2 = hand2;
        hand2 = c;
      } else {
        double p;
        if(br_switch && turn == br_player){
          p = 1.0;
        } else {
          p = w.infset_it->second.get_prob_action();
        }
#ifdef CFR
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
#ifdef CFR_SELF_PLAY
        if(cfr_switch){
          assert(w.infset_it->second.get_sum_i(1) != 0);
          p = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
        }
#endif
        npi[turn][depth+1] = npi[turn][depth] * p;
        npi[1-turn][depth+1] = npi[1-turn][depth];
        npi[2][depth+1] = npi[2][depth];
        rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
        org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
        addstr[0][naddstr[0]++] = action_to_char(4, 4); addstr[0][naddstr[0]++] = wizard_to_char(1-turn, 7, 7);
        addstr[2-turn][naddstr[2-turn]++] = action_to_char(4, 4); addstr[2-turn][naddstr[2-turn]++] = wizard_to_char(1-turn, 7, 7);
        addstr[1+turn][naddstr[1+turn]++] = action_to_char(4, 4); addstr[1+turn][naddstr[1+turn]++] = wizard_to_char(1-turn, 7, 7);
      }
      break;
    case 7: //wizard_self
      //if(0 >= c || c >= 9) print_history();
      assert(0 < c && c < 9);
      //if(deck[c-1] <= 0) {
      //  cout << a << " " << c << " " << deck[0] << " " << deck[1] << " " << deck[2] << " " << deck[3] << " " << deck[4] << " " << deck[5] << " " << deck[6] << " " << deck[7] << endl;
      //  print_history();
      //}
      assert(deck[c-1] > 0);
      w.prev_open1 = open1;
      open1 = 0;
      double ps;
      if(br_switch && turn == br_player){
        ps = 0.0;
      } else {
        ps = w.infset_it->second.get_prob_action();
      }
#ifdef CFR
      if(cfr_switch){
        assert(w.infset_it->second.get_sum_i(1) != 0);
        ps = w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
      }
#endif
#ifdef CFR_SELF_PLAY
      if(cfr_switch){
        assert(w.infset_it->second.get_sum_i(1) != 0);
        ps = (double)w.infset_it->second.get_sum_i(0) / w.infset_it->second.get_sum_i(1);
      }
#endif
      npi[turn][depth+1] = npi[turn][depth] * (1 - ps);
      npi[1-turn][depth+1] = npi[1-turn][depth];
      npi[2][depth+1] = npi[2][depth] * deck[c-1] * dsum_rec();
      rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
      org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
      addstr[0][naddstr[0]++] = action_to_char(4, 4); addstr[0][naddstr[0]++] = wizard_to_char(turn, hand1[0]-1, c-1);
      addstr[2-turn][naddstr[2-turn]++] = action_to_char(4, 4); addstr[2-turn][naddstr[2-turn]++] = wizard_to_char(turn, hand1[0]-1, 0);
      addstr[1+turn][naddstr[1+turn]++] = action_to_char(4, 4); addstr[1+turn][naddstr[1+turn]++] = wizard_to_char(turn, hand1[0]-1, c-1);
      deck[c-1] -= 1;
      w.prev_hand1 = hand1[0];
      hand1[0] = c;
      break;
    case 8: //soldior choise
      //if(0 >= c || c >= 9) print_history();
      assert(1 < c && c < 9);
      npi[0][depth+1] = npi[0][depth];
      npi[1][depth+1] = npi[1][depth];
      npi[2][depth+1] = npi[2][depth];
      org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
      addstr[0][naddstr[0]++] = action_to_char(4, 0); addstr[0][naddstr[0]++] = twonum_to_char(0, c - 1);
      addstr[1][naddstr[1]++] = action_to_char(4, 0); addstr[1][naddstr[1]++] = twonum_to_char(0, c - 1);
      addstr[2][naddstr[2]++] = action_to_char(4, 0); addstr[2][naddstr[2]++] = twonum_to_char(0, c - 1);
      break;
  }
  depth++;

  if(a == 8){
    addstr[0][naddstr[0]] = static_cast<char>(NULL);
    if(strlen(addstr[0]) > 0) org_his.push(&(addstr[0][0]), strlen(addstr[0]));
    addstr[1][naddstr[1]] = static_cast<char>(NULL);
    if(strlen(addstr[1]) > 0) org_his_p[0].push(&(addstr[1][0]), strlen(addstr[1]));
    addstr[2][naddstr[2]] = static_cast<char>(NULL);
    if(strlen(addstr[2]) > 0) org_his_p[1].push(&(addstr[2][0]), strlen(addstr[2]));
  } else {
    addstr[0][naddstr[0]] = static_cast<char>(NULL);
    if(strlen(addstr[0]) > 0) rnd_his.push(&(addstr[0][0]), strlen(addstr[0]));
    if(strlen(addstr[0]) > 0) org_his.push(&(addstr[0][0]), strlen(addstr[0]));
    addstr[1][naddstr[1]] = static_cast<char>(NULL);
    if(strlen(addstr[1]) > 0) rnd_his_p[0].push(&(addstr[1][0]), strlen(addstr[1]));
    if(strlen(addstr[1]) > 0) org_his_p[0].push(&(addstr[1][0]), strlen(addstr[1]));
    addstr[2][naddstr[2]] = static_cast<char>(NULL);
    if(strlen(addstr[2]) > 0) rnd_his_p[1].push(&(addstr[2][0]), strlen(addstr[2]));
    if(strlen(addstr[2]) > 0) org_his_p[1].push(&(addstr[2][0]), strlen(addstr[2]));
  }

  //print_node();
  //bool valid = valid_data();
  //if(!valid) cout << "do_action : " << a << " card : " << c << endl;
  assert(valid_data());
  return;
}

void node::undo_action(int a, int c, work_do_action &w){
  assert(0 < a && a < 9);
  switch(a) {
    case 1: //put_hide_card
      assert(0 < c && c < 9);
      deck[c-1] += 1; hide = 0;
      rnd_his.erase();
      org_his.erase();
      break;
    case 2: //draw_p1_init
      assert(0 < c && c < 9);
      deck[c-1] += 1; hand1[0] = 0;
      rnd_his.erase(); rnd_his_p[0].erase();
      org_his.erase(); org_his_p[0].erase();
      break;
    case 3: //draw_p2_init
      assert(0 < c && c < 9);
      deck[c-1] += 1; hand2 = 0;
      rnd_his.erase(); rnd_his_p[1].erase();
      org_his.erase(); org_his_p[1].erase();
      break;
    case 4: //draw
      assert(0 < c && c < 9);
      deck[c-1] += 1; hand1[1] = 0; count_turn--;
      rnd_his.erase(); rnd_his_p[turn].erase();
      org_his.erase(); org_his_p[turn].erase();
      if(count_turn != 0) { //change_turn
        turn = 1 - turn;
        std::swap(hand1[0], hand2);
        std::swap(open1, open2);
        std::swap(barrier1, barrier2);
        barrier2 = w.prev_barrier2;
      }
      break;
    case 5: //play
      assert(0 < c && c < 9);
      if(w.prev_play == 0){
        hand1[1] = hand1[0];
        hand1[0] = c;
      } else {
        hand1[1] = c;
      }
      switch(c){
        case 1:
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          break;
        case 2:
          if(barrier2 == false){
            open2 = w.prev_open2;
            rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
            org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          } else {
            rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
            org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          }
          break;
        case 3:
          if(hand1[1-w.prev_play] == hand2 && barrier2 == false) {
            open1 = w.prev_open1; open2 = w.prev_open2;
            rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
            org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          } else {
            rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
            org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          }
          break;
        case 4:
          barrier1 = 0;
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          break;
        case 5:
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          count_turn--;
          break;
        case 6:
          if(barrier2 == false){
            rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
            org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
            if(w.prev_play == 0){
              std::swap(hand1[1], hand2);
            } else {
              std::swap(hand1[0], hand2);
            }
            open1 = w.prev_open1; open2 = w.prev_open2;
          } else {
            rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
            org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          }
          break;
        case 7:
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          break;
        case 8:
          rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
          org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
          break;
      }
      if(c == w.used_open1) open1 = c;
      break;
    case 6: //wizard
      if(barrier2 == false){
        assert(0 < c && c < 9);
        rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
        org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
        char addstr[16];
        addstr[0] = action_to_char(4, 4);
        addstr[1] = static_cast<char>(NULL);
        rnd_his.push(&(addstr[0]), strlen(addstr));
        rnd_his_p[0].push(&(addstr[0]), strlen(addstr));
        rnd_his_p[1].push(&(addstr[0]), strlen(addstr));
        org_his.push(&(addstr[0]), strlen(addstr));
        org_his_p[0].push(&(addstr[0]), strlen(addstr));
        org_his_p[1].push(&(addstr[0]), strlen(addstr));
        deck[c-1] += 1;
        hand2 = w.prev_hand2;
        open2 = w.prev_open2;
      } else {
        rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
        org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
        char addstr[16];
        addstr[0] = action_to_char(4, 4);
        addstr[1] = static_cast<char>(NULL);
        rnd_his.push(&(addstr[0]), strlen(addstr));
        rnd_his_p[0].push(&(addstr[0]), strlen(addstr));
        rnd_his_p[1].push(&(addstr[0]), strlen(addstr));
        org_his.push(&(addstr[0]), strlen(addstr));
        org_his_p[0].push(&(addstr[0]), strlen(addstr));
        org_his_p[1].push(&(addstr[0]), strlen(addstr));
      }
      break;
    case 7: { //wizard_self
      assert(0 < c && c < 9);
      rnd_his.erase(); rnd_his_p[0].erase(); rnd_his_p[1].erase();
      org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
      char addstr[16];
      addstr[0] = action_to_char(4, 4);
      addstr[1] = static_cast<char>(NULL);
      rnd_his.push(&(addstr[0]), strlen(addstr));
      rnd_his_p[0].push(&(addstr[0]), strlen(addstr));
      rnd_his_p[1].push(&(addstr[0]), strlen(addstr));
      org_his.push(&(addstr[0]), strlen(addstr));
      org_his_p[0].push(&(addstr[0]), strlen(addstr));
      org_his_p[1].push(&(addstr[0]), strlen(addstr));
      deck[c-1] += 1;
      hand1[0] = w.prev_hand1;
      open1 = w.prev_open1;
      break; }
    case 8: {
      assert(0 < c && c < 9);
      org_his.erase(); org_his_p[0].erase(); org_his_p[1].erase();
      char addstr[16];
      addstr[0] = action_to_char(4, 0);
      addstr[1] = static_cast<char>(NULL);
      org_his.push(&(addstr[0]), strlen(addstr));
      org_his_p[0].push(&(addstr[0]), strlen(addstr));
      org_his_p[1].push(&(addstr[0]), strlen(addstr));
      break; }
  }
  depth--;
  //cout << "undo_action : " << a << " card : " << c << endl;
  //print_node();
  assert(valid_data());
  return;
}

void node::print_history()const noexcept{
  node n(open);
  long unsigned int head = 0;
  string history = org_his.get_hash_value();
  //string history = rnd_his.get_hash_value();
  while(head < history.size()){
    string action = oph.get_action((unsigned char)history[head]);
    //string action = rph.get_action((unsigned char)history[head]);
    int c2a = char_to_action(action[0]); head++;
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    switch(c2a1){
      case 0:
        cout << "put_hide_card : " << c2a2 + 1 << endl;
        n.hide = c2a2 + 1;
        n.deck[c2a2]--;
        break;
      case 1:
        cout << "draw_p1_init : " << c2a2 + 1 << endl;
        n.hand1[0] = c2a2 + 1;
        n.deck[c2a2]--;
        break;
      case 2:
        cout << "draw_p2_init : " << c2a2 + 1 << endl;
        n.hand2 = c2a2 + 1;
        n.deck[c2a2]--;
        break;
      case 3:
        if(n.count_turn != 0){
          n.turn = 1 - n.turn;
          std::swap(n.hand1[0], n.hand2);
          std::swap(n.open1, n.open2);
          n.barrier2 = false;
          std::swap(n.barrier1, n.barrier2);
        }
        cout << "player " << n.turn << " draw : " << c2a2 + 1 << endl;
        n.hand1[1] = c2a2 + 1;
        n.deck[c2a2] -= 1;
        n.count_turn++;
        break;
      case 4:
        if(c2a2 + 1 == n.open1) n.open1 = 0;
        if((use_same_move && n.hand1[0] == n.hand1[1]) || (nuse_bad_move && ((n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) || n.hand1[0] == 8))){
          n.hand1[1] = 0;
        } else if(nuse_bad_move && ((n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) || n.hand1[1] == 8)){
          n.hand1[0] = n.hand1[1]; n.hand1[1] = 0;
        } else if(n.hand1[0] == c2a2 + 1){
          n.hand1[0] = n.hand1[1]; n.hand1[1] = 0;
        } else {
          n.hand1[1] = 0;
        }
        cout << "player " << n.turn << " play : " << c2a2 + 1 << endl;
        if(c2a2 + 1 == 1 && n.barrier2 == false && action.size() > 1){
          int c2t = char_to_twonum(action[1]);
          int c2t2 = c2t % 10;
          cout << "soldior : " << c2t2 + 1 << endl;
        } else if(c2a2 + 1 == 2 && n.barrier2 == false){
          int c2t = char_to_twonum(action[1]);
          int c2t2 = c2t % 10;
          n.open2 = c2t2 + 1;
          cout << "open2 : " << c2t2 + 1 << endl;
        } else if(c2a2 + 1 == 3 && n.barrier2 == false && n.hand1[0] == n.hand2){
          n.open1 = n.hand1[0]; n.open2 = n.hand2; head++;
          cout << "same " << n.open1 << endl;
        } else if(c2a2 + 1 == 4){
          n.barrier1 = true;
        } else if(c2a2 + 1 == 5){
          if(n.barrier2 == false){
            if(head < history.size()){
              int c2w = char_to_wizard(action[1]);
              int to = c2w / 100;
              int trash = (c2w / 10) % 10;
              int draw = c2w % 10;
              cout << "player " << to << " trash " << trash + 1 << " and get " << draw + 1 << endl;
              if((to == 1 && n.turn == 0) || (to == 0 && n.turn == 1)){
                n.open2 = 0;
                n.hand2 = draw + 1;
              } else {
                n.open1 = 0;
                n.hand1[0] = draw + 1;
              }
              n.count_turn++;
              n.deck[draw] -= 1;
            }
          } else {
            if(head < history.size()){
              int c2w = char_to_wizard(action[1]);
              int to = c2w / 100;
              int trash = (c2w / 10) % 10;
              int draw = c2w % 10;
              if(trash == 7 && draw == 7){
                cout << "player " << to << " don't trash." << endl;
              } else {
                cout << "player " << to << " trash " << trash + 1 << " and get " << draw + 1 << endl;
                n.open1 = 0;
                n.hand1[0] = draw + 1;
                n.deck[draw] -= 1;
              }
              n.count_turn++;
            }
          }
        } else if(c2a2 + 1 == 6 && n.barrier2 == false){
          int c2t = char_to_twonum(action[1]);
          int card1 = (c2t / 10) + 1;
          int card2 = (c2t % 10) + 1;
          cout << "give " << card1 << " and get " << card2 << endl;
          n.hand1[0] = card2; n.open1 = card2;
          n.hand2 = card1; n.open2 = card1;
        }
        break;
    }
  }
}

void node::print_history_rnd()const noexcept{
  node n(open);
  long unsigned int head = 0;
  string history = rnd_his.get_hash_value();
  //string history = rnd_his.get_hash_value();
  while(head < history.size()){
    string action = rph.get_action((unsigned char)history[head]);
    //string action = rph.get_action((unsigned char)history[head]);
    int c2a = char_to_action(action[0]); head++;
    int c2a1 = c2a / 10; int c2a2 = c2a % 10;
    switch(c2a1){
      case 0:
        cout << "put_hide_card : " << c2a2 + 1 << endl;
        n.hide = c2a2 + 1;
        n.deck[c2a2]--;
        break;
      case 1:
        cout << "draw_p1_init : " << c2a2 + 1 << endl;
        n.hand1[0] = c2a2 + 1;
        n.deck[c2a2]--;
        break;
      case 2:
        cout << "draw_p2_init : " << c2a2 + 1 << endl;
        n.hand2 = c2a2 + 1;
        n.deck[c2a2]--;
        break;
      case 3:
        if(n.count_turn != 0){
          n.turn = 1 - n.turn;
          std::swap(n.hand1[0], n.hand2);
          std::swap(n.open1, n.open2);
          n.barrier2 = false;
          std::swap(n.barrier1, n.barrier2);
        }
        cout << "player " << n.turn << " draw : " << c2a2 + 1 << endl;
        n.hand1[1] = c2a2 + 1;
        n.deck[c2a2] -= 1;
        n.count_turn++;
        break;
      case 4:
        if(c2a2 + 1 == n.open1) n.open1 = 0;
        if((use_same_move && n.hand1[0] == n.hand1[1]) || (nuse_bad_move && ((n.hand1[0] == 3 && n.hand1[1] < n.open2 && n.barrier2 == false) || n.hand1[0] == 8))){
          n.hand1[1] = 0;
        } else if(nuse_bad_move && ((n.hand1[0] < n.open2 && n.hand1[1] == 3 && n.barrier2 == false) || n.hand1[1] == 8)){
          n.hand1[0] = n.hand1[1]; n.hand1[1] = 0;
        } else if(n.hand1[0] == c2a2 + 1){
          n.hand1[0] = n.hand1[1]; n.hand1[1] = 0;
        } else {
          n.hand1[1] = 0;
        }
        cout << "player " << n.turn << " play : " << c2a2 + 1 << endl;
        if(c2a2 + 1 == 1 && n.barrier2 == false && action.size() > 1){
          int c2t = char_to_twonum(action[1]);
          int c2t2 = c2t % 10;
          cout << "soldior : " << c2t2 + 1 << endl;
        } else if(c2a2 + 1 == 2 && n.barrier2 == false){
          int c2t = char_to_twonum(action[1]);
          int c2t2 = c2t % 10;
          n.open2 = c2t2 + 1;
          cout << "open2 : " << c2t2 + 1 << endl;
        } else if(c2a2 + 1 == 3 && n.barrier2 == false && n.hand1[0] == n.hand2){
          n.open1 = n.hand1[0]; n.open2 = n.hand2; head++;
          cout << "same " << n.open1 << endl;
        } else if(c2a2 + 1 == 4){
          n.barrier1 = true;
        } else if(c2a2 + 1 == 5){
          if(n.barrier2 == false){
            if(head < history.size()){
              int c2w = char_to_wizard(action[1]);
              int to = c2w / 100;
              int trash = (c2w / 10) % 10;
              int draw = c2w % 10;
              cout << "player " << to << " trash " << trash + 1 << " and get " << draw + 1 << endl;
              if((to == 1 && n.turn == 0) || (to == 0 && n.turn == 1)){
                n.open2 = 0;
                n.hand2 = draw + 1;
              } else {
                n.open1 = 0;
                n.hand1[0] = draw + 1;
              }
              n.count_turn++;
              n.deck[draw] -= 1;
            }
          } else {
            if(head < history.size()){
              int c2w = char_to_wizard(action[1]);
              int to = c2w / 100;
              int trash = (c2w / 10) % 10;
              int draw = c2w % 10;
              if(trash == 7 && draw == 7){
                cout << "player " << to << " don't trash." << endl;
              } else {
                cout << "player " << to << " trash " << trash + 1 << " and get " << draw + 1 << endl;
                n.open1 = 0;
                n.hand1[0] = draw + 1;
                n.deck[draw] -= 1;
              }
              n.count_turn++;
            }
          }
        } else if(c2a2 + 1 == 6 && n.barrier2 == false){
          int c2t = char_to_twonum(action[1]);
          int card1 = (c2t / 10) + 1;
          int card2 = (c2t % 10) + 1;
          cout << "give " << card1 << " and get " << card2 << endl;
          n.hand1[0] = card2; n.open1 = card2;
          n.hand2 = card1; n.open2 = card1;
        }
        break;
    }
  }
}

void node::print_node(){
  cout << "history" << endl;
  print_history();
  cout << "hide : " << hide << " open : " << open[0] << " " << open[1] << " " << open[2] << endl;
  cout << "deck : " << deck[0] << " " << deck[1] << " " << deck[2] << " " << deck[3] << " " << deck[4] << " " << deck[5] << " " << deck[6] << " " << deck[7] << endl;
  cout << "depth : " << depth << endl;
  cout << "turn : " << turn << endl;
  cout << "hand1 : " << hand1[0] << " " << hand1[1] << " hand2 : " << hand2 << endl;
  cout << "open1 : " << open1 << " open2 : " << open2 << endl;
  cout << "barrier1 : " << barrier1 << " barrier2 : " << barrier2 << endl;
  cout << "----------------------------" << endl;
  return;
}