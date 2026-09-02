// CFR
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

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "org_action.hpp"
#include "loveletter.hpp"

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);

using namespace std;

const double table_sign[2] = {1.0, -1.0};
const char action_sign[8] = {'0', 'a', 'c', 'd', 'e', 'f', 'g', 'h'};
const char card_sign[8][20] = {"兵士", "道化", "騎士", "僧侶", "魔術師", "将軍", "大臣", "姫"};
int cfr_switch = 0;
int cfr_player = 0;
bool br_switch = false;
int br_player = 0;
bool org_switch = false;
map<std::string, infset> table_infset{};
int base = 2;
int MAX_index = 10;
int MAX_interval = 300;
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;
bool knight_check = true;
static Rnd_Perfect_Hash rph;

#include "cfr_exp_reward.hpp"
#include "rnd_make_infset.hpp"
#include "cfr.hpp"

void cfr(int t0, int t, int open[3]) {
  string filename0 = "str";
  string subgame0 = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  string iter0 = to_string(t0);
  filename0 = filename0 + subgame0 + iter0 + ".bin";
  cout << filename0 << endl;
  ifstream input_file0(filename0, ios::in | ios::binary);
  if(!input_file0) {
    cerr << "file read error" << endl;
    terminate();
  }
  string filename1 = "str";
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  string iter = to_string(t0 + t);
  filename1 = filename1 + subgame + iter + ".bin";
  cout << filename1 << endl;
  ofstream output_file1(filename1, ios::out | ios::binary);

  node n_ds(open);
  double R;
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n_ds.deck[i - 1] == 0) {
      continue;
    }
    work_do_action ds_w;
    n_ds.do_action(1, i, ds_w);
    rnd_ds_put_hide_card(n_ds);
    n_ds.undo_action(1, i, ds_w);
    cout << table_infset.size() << endl;
  }
  cout << "p1_points : " << p1_points << endl;
  cout << "p2_points : " << p2_points << endl;
  cout << "rand_points : " << rand_points << endl;
  cout << "end_points : " << end_points << endl;
  cout << "End DS. " << endl;

  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
    float sum_i0;
    float sum_i1;
    float regret0;
    float regret1;
    input_file0.read((char *)&sum_i0, sizeof(float));
    input_file0.read((char *)&sum_i1, sizeof(float));
    input_file0.read((char *)&regret0, sizeof(float));
    input_file0.read((char *)&regret1, sizeof(float));
    it->second.set_sum_i(0, (double)sum_i0);
    it->second.set_sum_i(1, (double)sum_i1);
    it->second.set_regret(0, (double)regret0);
    it->second.set_regret(1, (double)regret1);
    string action = rph.get_action((unsigned char)it->first[0]);
    int c2a = char_to_action(action[0]);
    if((c2a / 10 == 1) && (max(it->second.get_regret(0), 0.0) + max(it->second.get_regret(1), 0.0) > 0)) {
      it->second.set_prob_action(max(it->second.get_regret(0), 0.0) / (max(it->second.get_regret(0), 0.0) + max(it->second.get_regret(1), 0.0)));
    } else if((c2a / 10 == 2) && (min(it->second.get_regret(0), 0.0) + min(it->second.get_regret(1), 0.0) < 0)) {
      it->second.set_prob_action(min(it->second.get_regret(0), 0.0) / (min(it->second.get_regret(0), 0.0) + min(it->second.get_regret(1), 0.0)));
    } else {
      it->second.set_prob_action(0.5);
    }
  }
  cout << "End make_infset." << endl;

  bool ccc;
  for(int k = 15; k > 0; k--) {
    ccc = true;
    int is_per_d = 0;
    for(map<std::string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
      if(it->second.depth == k) {
        if(ccc) ccc = false;
        is_per_d++;
      }
    }
    if(!ccc) cout << "depth " << k << " : " << is_per_d << endl;
  }

  for(int i = 1; i <= t; i++) {
    // CFR戦略の導出
    cfr_switch = 0;
    node n(open);
    R = 0.0;
    double p1 = n.dsum_rec();
    for(int j = 1; j < 9; j++) {
      if(n.deck[j - 1] == 0) {
        continue;
      }
      work_do_action w;
      double p2 = n.deck[j - 1];
      n.do_action(1, j, w);
      R += put_hide_card(n) * p1 * p2;
      n.undo_action(1, j, w);
    }

    for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
      string action = rph.get_action((unsigned char)it->first[0]);
      int c2a = char_to_action(action[0]);
      if((c2a / 10 == 1) && (max(it->second.get_regret(0), 0.0) + max(it->second.get_regret(1), 0.0) > 0)) {
        it->second.set_prob_action(max(it->second.get_regret(0), 0.0) / (max(it->second.get_regret(0), 0.0) + max(it->second.get_regret(1), 0.0)));
      } else if((c2a / 10 == 2) && (min(it->second.get_regret(0), 0.0) + min(it->second.get_regret(1), 0.0) < 0)) {
        it->second.set_prob_action(min(it->second.get_regret(0), 0.0) / (min(it->second.get_regret(0), 0.0) + min(it->second.get_regret(1), 0.0)));
      } else {
        it->second.set_prob_action(0.5);
      }
      it->second.set_sum_i(0, it->second.get_sum_i(0) + it->second.get_pi_i() * it->second.get_prob_action());
      it->second.set_sum_i(1, it->second.get_sum_i(1) + it->second.get_pi_i());
      assert(it->second.get_sum_i(1) > 0);
    }
    if(i < 10 || (i % 10 == 0 && i > 10 && i < 100) || (i % 100 == 0 && i > 100 && i < 1000) || (i % 1000 && i > 1000 && i < 10000)) {
      cout << "iteration : " << i << endl;
    }
  }

  if(t > 0) {
    //情報集合ごとの平均戦略の出力
    for(map<std::string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
      float sum_i0 = (float)it->second.get_sum_i(0);
      float sum_i1 = (float)it->second.get_sum_i(1);
      float regret0 = (float)it->second.get_regret(0);
      float regret1 = (float)it->second.get_regret(1);
      output_file1.write((char *)&sum_i0, sizeof(float));
      output_file1.write((char *)&sum_i1, sizeof(float));
      output_file1.write((char *)&regret0, sizeof(float));
      output_file1.write((char *)&regret1, sizeof(float));
    }

    //根節点の期待利得
    cfr_switch = 1;
    node n_ut(open);
    double A = 0;
    double p1 = n_ut.dsum_rec();
    for(int i = 1; i < 9; i++) {
      if(n_ut.deck[i - 1] == 0) {
        continue;
      }
      work_do_action ut_w;
      double p2 = n_ut.deck[i - 1];
      n_ut.do_action(1, i, ut_w);
      A += ut_put_hide_card(n_ut) * p1 * p2;
      n_ut.undo_action(1, i, ut_w);
    }
    cout << "Utility of Average prob_action = " << A << std::endl;
  }

  if(!output_file1) {
    cerr << "file write error" << endl;
    terminate();
  }
  output_file1.close();
  return;
}

int main(int, char *argv[]) {
  int t0, t, a, b, c;
  t0 = atoi(argv[1]);
  t = atoi(argv[2]);
  a = atoi(argv[3]);
  b = atoi(argv[4]);
  c = atoi(argv[5]);

  cout << "iteration : " << t << endl;
  cout << "open : " << a << " " << b << " " << c << endl;

  int open[3] = {a, b, c};
  cfr(t0, t, open);

  return 0;
}
