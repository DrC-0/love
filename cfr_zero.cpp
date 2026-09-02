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
double pi_i;
int base = 2;
int MAX_index = 10;
int MAX_interval = 300;
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;
bool knight_check = true;

#include "rnd_make_infset.hpp"

void cfr_zero(int open[3]) {
  string filename1 = "str";
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  string iter = to_string(0);
  filename1 = filename1 + subgame + iter + ".bin";
  cout << filename1 << endl;
  ofstream output_file1(filename1, ios::out | ios::binary);

  string filename2 = "utilities";
  filename2 = filename2 + subgame + ".csv";
  cout << filename2 << endl;
  ofstream output_file2(filename2, ios::out);
  int init_deck[8] = {5, 2, 2, 2, 2, 1, 1, 1};
  double subgame_prob;
  if(open[0] == 1 && open[0] == open[1] && open[1] == open[2]) {
    subgame_prob = (double)10 / 560;
  } else if(open[0] == open[1]) {
    if(open[0] == 1) {
      subgame_prob = (double)10 * init_deck[open[2] - 1] / 560;
    } else {
      subgame_prob = (double)init_deck[open[2] - 1] / 560;
    }
  } else if(open[1] == open[2]) {
    if(open[1] == 1) {
      subgame_prob = (double)10 * init_deck[open[0] - 1] / 560;
    } else {
      subgame_prob = (double)init_deck[open[0] - 1] / 560;
    }
  } else {
    subgame_prob = (double)init_deck[open[0] - 1] * init_deck[open[1] - 1] * init_deck[open[2] - 1] / 560;
  }
  output_file2 << open[0] << open[1] << open[2] << "," << subgame_prob << endl;

  node n_ds(open);
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
    it->second.set_sum_i(0, it->second.get_sum_i(0) + it->second.get_pi_i() * it->second.get_prob_action());
    it->second.set_sum_i(1, it->second.get_sum_i(1) + it->second.get_pi_i());
    assert(it->second.get_sum_i(1) > 0);
  }

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
  if(!output_file1) {
    cerr << "file write error" << endl;
    terminate();
  }
  output_file1.close();
  return;
}

int main(int, char *argv[]) {
  int a, b, c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);

  cout << "iteration : " << 0 << endl;
  cout << "open : " << a << " " << b << " " << c << endl;

  int open[3] = {a, b, c};
  cfr_zero(open);

  return 0;
}
