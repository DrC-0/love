// BEST RESPONSE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <map>
#include <ctime>
#include <cstdlib>
#include <random>
#include <vector>
#include <string>
#include <numeric>
#include <cassert>
#include <exception>
#include <algorithm>
#include <stack>
#include <set>

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "org_action.hpp"
#include "loveletter.hpp"

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);

using namespace std;

void output_history(string s);
void output_hash_history(string s, bool rnd);

const double table_sign[2] = {1.0, -1.0};
const char action_sign[8] = {'0', 'a', 'c', 'd', 'e', 'f', 'g', 'h'};
const char card_sign[8][20] = {"兵士", "道化", "騎士", "僧侶", "魔術師", "将軍", "大臣", "姫"};
int cfr_switch = 0;
int cfr_player = 0;
bool br_switch = false;
int br_player = 0;
bool org_switch = false;
map<std::string, infset> table_infset{};
map<std::string, double> table_exp_reward{};
stack<string> stack_infset;
vector<string> all_history;
double soldior_prob[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double random_soldior_prob[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;
unsigned long int soldior_infsets = 0;
unsigned long int soldior_points = 0;
static Rnd_Perfect_Hash rph;
static Org_Perfect_Hash oph;

#include "all_elements_rnd.hpp"
#include "rnd_make_infset.hpp"
#include "infset_dfs.hpp"
#include "save_load_abshistory.hpp"
#include "bf_position.hpp"

void output_hash_history(string s, bool rnd) {
  long unsigned int head = 0;
  int turn = 0;
  bool bal[2] = {false, false};
  while(head < s.size()) {
    string action;
    if(rnd) {
      action = rph.get_action((unsigned char)s[head]);
    } else {
      action = oph.get_action((unsigned char)s[head]);
    }
    int c2a = char_to_action(action[0]);
    head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;
    cout << action_sign[num1 + 1] << num2 + 1;
    bal[turn] = false;
    if(num1 == 4) {
      if(br_player == turn && num2 == 0 && bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        cout << (c2t % 10) + 1;
      } else if(num2 == 1 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        cout << (c2t % 10) + 1;
      } else if(num2 == 2 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        cout << (c2t % 10) + 1;
      } else if(num2 == 3) {
        bal[turn] = true;
      } else if(num2 == 4) {
        int c2w = char_to_wizard(action[1]);
        cout << c2w / 100 << ((c2w / 10) % 10) + 1 << (c2w % 10) + 1;
      } else if(num2 == 5 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        cout << (c2t / 10) + 1 << (c2t % 10) + 1;
      }
      turn = 1 - turn;
    }
  }
}

void compare_abs_cfr(int open[3]) {
  string filename1 = "str";
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  filename1 = filename1 + subgame + "64.bin";
  cout << filename1 << endl;
  ifstream input_file1(filename1, ios::in | ios::binary);
  if(!input_file1) {
    cerr << "file read error" << endl;
    terminate();
  }

  node n_rnd_ds(open);
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n_rnd_ds.deck[i - 1] == 0) {
      continue;
    }
    work_do_action ds_w;
    n_rnd_ds.do_action(1, i, ds_w);
    rnd_ds_put_hide_card(n_rnd_ds);
    n_rnd_ds.undo_action(1, i, ds_w);
    cout << table_infset.size() << endl;
  }
  cout << "End Rnd_DS." << endl;

  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();) {
    float sum_i0;
    float sum_i1;
    float regret0;
    float regret1;
    input_file1.read((char*)&sum_i0, sizeof(float));
    input_file1.read((char*)&sum_i1, sizeof(float));
    input_file1.read((char*)&regret0, sizeof(float));
    input_file1.read((char*)&regret1, sizeof(float));
    double ave_strategy = (double)sum_i0 / (double)sum_i1;
    it->second.set_prob_action(ave_strategy);
    ++it;
  }
  cout << "End make_infset." << endl;

  map<string, bool> abs_history;
  set<string> only_history;
  string filename2 = "abs" + subgame + ".bin";
  load_bin_abs(filename2, abs_history, only_history);
  // double err = 1e-2;

  map<string, bool>::iterator his;
  for(auto h = std::next(abs_history.begin()); h != abs_history.end(); ++h) {
    his = h;
    if(his->second) break;
  }
  output_actions_history(his->first, true);
  output_hash_history(his->first, true);
  auto bfp = bf_position(open, his->first);
  bfp.print();
  cout << "win" << is_win(bfp).first << endl;
  node n(open);
  string key = his->first;
  for(int i = 1; i < 9; i++) {
    if(n.deck[i - 1] == 0) {
      continue;
    }
    work_do_action all_w;
    n.do_action(1, i, all_w);
    all_put_hide_card(key, 0, n);
    n.undo_action(1, i, all_w);
  }
  for(auto& history : all_history) {
    output_actions_history(history, true);
    vector<string> m;
    node hist_n(history, true, open);
    string hist_key = hist_n.org_his_p[0].get_hash_value();
    all_exp_reward(hist_key, open, m, -1, 0);
    // map<string, double>::iterator table_exp_reward_it;
    // table_exp_reward_it = table_exp_reward.find(key);
    // cout << "exp_reward : " << table_exp_reward_it->second << endl;
  }
}

int main(int, char* argv[]) {
  int a, b, c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);

  cout << "open : " << a << " " << b << " " << c << endl;

  int open[3] = {a, b, c};
  compare_abs_cfr(open);

  return 0;
}