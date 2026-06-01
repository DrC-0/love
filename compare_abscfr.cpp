//BEST RESPONSE
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
#include<exception>
#include<algorithm>
#include<stack>
#include<set>

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
const char card_sign[8][20] = { "兵士", "道化", "騎士", "僧侶", "魔術師", "将軍", "大臣", "姫" };
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

#include "rnd_make_infset.hpp"
#include "save_load_abshistory.hpp"
#include "bf_position.hpp"

void compare_abs_cfr(int open[3]){
  string filename1 = "str";
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  filename1 = filename1 + subgame + "64.bin";
  cout << filename1 << endl;
  ifstream input_file1(filename1, ios::in | ios::binary);
  if(!input_file1){
    cerr << "file read error" << endl;
    terminate();
  }

  node n_rnd_ds(open);
  rand_points++;
  for(int i = 1; i < 9; i++){
    if(n_rnd_ds.deck[i-1] == 0) { continue; }
    work_do_action ds_w;
    n_rnd_ds.do_action(1, i, ds_w);
    rnd_ds_put_hide_card(n_rnd_ds);
    n_rnd_ds.undo_action(1, i, ds_w);
    cout << table_infset.size() << endl;
  }
  cout << "End Rnd_DS." << endl;

  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();){
    float sum_i0;
    float sum_i1;
    float regret0;
    float regret1;
    input_file1.read((char*) &sum_i0, sizeof(float));
    input_file1.read((char*) &sum_i1, sizeof(float));
    input_file1.read((char*) &regret0, sizeof(float));
    input_file1.read((char*) &regret1, sizeof(float));
    double ave_strategy = (double)sum_i0 / (double)sum_i1;
    it->second.set_prob_action(ave_strategy);
    ++it;
  }
  cout << "End make_infset." << endl;

  map<string, bool> abs_history;
  set<string> only_history;
  string filename2 = "abs" + subgame + ".bin";
  load_bin_abs(filename2, abs_history, only_history);
  double err = 1e-2;

  int results[2] = {0, 0};
  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();++it){
    // abs_historyに含まれていたら必勝あるいは必敗
    auto ut = abs_history.upper_bound(it->first);
    if (ut != abs_history.begin()) {
      auto ut_prev = std::prev(ut);
      if (it->first == ut_prev->first) {
        bf_position bfp(open, it->first);
        node n(it->first, true, open);
        if(ut_prev->second) {
          auto res = is_win(bfp);
          int casted = static_cast<int>(std::round(it->second.get_prob_action()));
          if(res.first == n.hand1[casted] && std::abs(casted - it->second.get_prob_action()) <= err){
            if(n.hand1[0] != bfp.hand0[0]){
              // cout << "win move:" << res.first << " :" << casted << " " << it->second.get_prob_action() << endl;
              output_actions_history(it->first, true);
              n.print_node();
              bfp.print();
              cout << "-----------------------------" << endl;
            }
            results[1]++;
          }else results[0]++;
        }
      } else if(only_history.contains(it->first)){
        // bf_position bfp(open, it->first);
        // auto lose_actions = is_lose(bfp);
        // for(auto lose_action : lose_actions){
        //   if(lose_action.first == bfp.hand0[0] && it->second.get_prob_action() < 1 - err)
        //     output_actions_history(it->first, true);
        //   else if(lose_action.first == bfp.hand0[1] && it->second.get_prob_action() > err)
        //     output_actions_history(it->first, true);
        // }
      }
    }
  }
  cout << "Results: " << results[0] << " : " << results[1] << endl;
}

int main(int argc, char *argv[]){
  int a, b, c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);

  cout << "open : " << a << " " << b << " " << c << endl;

  int open[3] = {a, b, c};
  compare_abs_cfr(open);

  return 0;
}