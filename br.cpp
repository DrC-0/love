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

#include "all_elements.hpp"
#include "rnd_make_infset.hpp"
#include "infset_dfs.hpp"

void output_history(std::string s) {
  long unsigned int head = 0;
  int turn = 0;
  bool sol = false;
  bool cra = false;
  bool kni = false;
  bool bal[2] = {false, false};
  bool wiz = false;
  bool gen = false;
  while(head < s.size()) {
    int c2w = char_to_wizard(s[head]);
    head++;
    int to = c2w / 100;
    int num1 = ((c2w / 10) % 10) + 1;
    int num2 = (c2w % 10) + 1;
    if(sol && !bal[turn]) {
      cout << num2;
      sol = false;
    } else if(cra && !bal[turn]) {
      cout << num2;
      cra = false;
    } else if(kni && !bal[turn]) {
      cout << num1;
      kni = false;
    } else if(wiz) {
      cout << to << num1 << num2;
      wiz = false;
    } else if(gen && !bal[turn]) {
      cout << num1 << num2;
      gen = false;
    } else {
      cout << action_sign[num1] << num2;
      cra = false;
      kni = false;
      bal[turn] = false;
      wiz = false;
      gen = false;
      if(num1 == 5 && num2 == 1) sol = true;
      if(num1 == 5 && num2 == 2) cra = true;
      if(num1 == 5 && num2 == 3) kni = true;
      if(num1 == 5 && num2 == 4) bal[turn] = true;
      if(num1 == 5 && num2 == 5) wiz = true;
      if(num1 == 5 && num2 == 6) gen = true;
      if(num1 == 5) turn = 1 - turn;
    }
  }
  cout << endl;
}

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

void best_response(bool brp1, int t, int open[3]) {
  string filename1 = "str";
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  string iter = to_string(t);
  filename1 = filename1 + subgame + iter + ".bin";
  cout << filename1 << endl;
  ifstream input_file1(filename1, ios::in | ios::binary);
  if(!input_file1) {
    cerr << "file read error" << endl;
    terminate();
  }
  string filename2 = "utilities";
  filename2 = filename2 + subgame + ".csv";
  cout << filename2 << endl;
  ofstream output_file2(filename2, ios::app);

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

  // all_elements Debug
#ifdef ALL_ELEMENTS_TEST
  cout << "Start all elements test." << endl;
  if(all_elements_test) {
    unsigned long int test_progress = 0;
    for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
      action_sequense ac;
      ac.set(it->first);
      string history_private = ac.get_hash_value();
      all_history.clear();
      node n_all_test(open);
      for(int i = 1; i < 9; i++) {
        if(n_all_test.deck[i - 1] == 0) {
          continue;
        }
        work_do_action all_test_w;
        n_all_test.do_action(1, i, all_test_w);
        all_put_hide_card(history_private, 0, n_all_test);
        n_all_test.undo_action(1, i, all_test_w);
      }
      sort(all_history.begin(), all_history.end());
      sort(it->second.history.begin(), it->second.history.end());
      if(all_history.size() != it->second.history.size()) {
        output_hash_history(ac.get_hash_value());
        cout << "Size mismatch. Original : " << it->second.history.size() << ", all_history : " << all_history.size() << endl;
        for(std::vector<std::string>::iterator h = it->second.history.begin(); h != it->second.history.end(); ++h) {
          output_hash_history(*h);
        }
        exit(1);
      }
      if(all_history != it->second.history) {
        output_hash_history(it->first);
        cout << "Vector mismatch." << endl;
        exit(1);
      }
      test_progress++;
      if(test_progress % 100000 == 0) cout << test_progress << " / " << table_infset.size() << endl;
    }
  }
  cout << "All elements test complete." << endl;
#endif
  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();) {
    float sum_i0;
    float sum_i1;
    float regret0;
    float regret1;
    input_file1.read((char *)&sum_i0, sizeof(float));
    input_file1.read((char *)&sum_i1, sizeof(float));
    input_file1.read((char *)&regret0, sizeof(float));
    input_file1.read((char *)&regret1, sizeof(float));
    string action = rph.get_action((unsigned char)it->first[0]);
    int c2a = char_to_action(action[0]);
    int first = c2a / 10;
    if((first == 1 && brp1) || (first == 2 && !brp1)) {
      it = table_infset.erase(it);
    } else {
      double ave_strategy = (double)sum_i0 / (double)sum_i1;
      it->second.set_prob_action(ave_strategy);
      ++it;
    }
  }
  cout << "End make_infset." << endl;

  /*
  //根節点の期待利得
    cfr_switch = 1;
    node n_ut(open);
    double A = 0;
    double p1 = n_ut.dsum_rec();
    for(int i = 1; i < 9; i++){
      if(n_ut.deck[i-1] == 0) { continue; }
      //if(i != 1) continue;
      work_do_action ut_w;
      double p2 = n_ut.deck[i-1];
      n_ut.do_action(1, i, ut_w);
      A += ut_put_hide_card(n_ut) * p1 * p2;
      n_ut.undo_action(1, i, ut_w);
    }
    cout << "Utility of Average prob_action = " << A << std::endl;
    return;
    */

  if(brp1) {
    br_player = 0;
  } else {
    br_player = 1;
  }
  br_switch = true;
  org_switch = true;
  double p1, p2;
  node n_exp(open);
  double R = 0.0;
  p1 = n_exp.dsum_rec();
  for(int l = 1; l < 9; l++) {
    if(n_exp.deck[l - 1] == 0) {
      continue;
    }
    work_do_action exp_w;
    p2 = n_exp.deck[l - 1];
    n_exp.do_action(1, l, exp_w);
    R += infset_dfs_put_hide_card(n_exp, all_history, -1) * p1 * p2;
    n_exp.undo_action(1, l, exp_w);
    cout << l << " : end" << endl;
  }
  if(brp1) {
    cout << "P1 : best responce, P2 : cfr" << endl;
    cout << "P2 expected reward = " << R << endl;
    cout << endl;
    cout << "P2" << endl;
    output_file2 << t << "," << R;
  } else {
    cout << "P1 : cfr, P2 : best responce" << endl;
    cout << "P1 expected reward = " << R << endl;
    cout << endl;
    cout << "P1" << endl;
    output_file2 << "," << R << endl;
  }
  cout << "Prob having card (used soldior) :" << endl;
  double sum_soldior_prob = 0;
  for(int i = 0; i < 8; i++) sum_soldior_prob += soldior_prob[i];
  for(int i = 0; i < 8; i++) cout << i + 1 << " : " << soldior_prob[i] / sum_soldior_prob << endl;
  cout << endl;
  cout << "Prob choosing card (use soldior) :" << endl;
  double sum_random_soldior_prob = 0;
  for(int i = 1; i < 8; i++) sum_random_soldior_prob += random_soldior_prob[i - 1];
  for(int i = 1; i < 8; i++) cout << i + 1 << " : " << random_soldior_prob[i - 1] / sum_random_soldior_prob << endl;
  return;
}

int main(int argc, char *argv[]) {
  int p, t, a, b, c;
  p = atoi(argv[1]);
  t = atoi(argv[2]);
  a = atoi(argv[3]);
  b = atoi(argv[4]);
  c = atoi(argv[5]);

  bool brp1;
  if(p == 1) {
    brp1 = true;
    cout << "P1 best responce." << endl;
  } else if(p == 2) {
    brp1 = false;
    cout << "P2 best responce." << endl;
  } else {
    terminate();
  }
  cout << "open : " << a << " " << b << " " << c << endl;

  int open[3] = {a, b, c};
  best_response(brp1, t, open);

  return 0;
}