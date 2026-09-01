// CFR
#include <iostream>
#include <map>
#include <cstdlib>
#include <vector>
#include <string>
#include <cassert>
#include <numeric>

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "org_action.hpp"
#include "loveletter.hpp"

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);

using namespace std;

int cfr_switch = 0;
bool br_switch = false;
int br_player = 0;
bool org_switch = true;
map<std::string, infset> table_infset{};
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;
unsigned long int win_points[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long int lose_points[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long int decision_points[4] = {0, 0, 0, 0};
unsigned long int opengame = 0;
static Org_Perfect_Hash oph;

#include "make_infset.hpp"

void cfr_org(int open[3]) {
  node n_ds(open);
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n_ds.deck[i - 1] == 0) {
      continue;
    }
    work_do_action ds_w;
    n_ds.do_action(1, i, ds_w);
    org_ds_put_hide_card(n_ds);
    n_ds.undo_action(1, i, ds_w);
    cout << decision_points[0] << endl;
  }
  cout << "p1_points : " << p1_points << endl;
  cout << "p2_points : " << p2_points << endl;
  cout << "rand_points : " << rand_points << endl;
  cout << "end_points : " << end_points << endl;
  cout << "win points:" << endl;
  for(int i = 0; i < 11; i++) cout << win_points[i] << " ";
  cout << endl;
  cout << "lose points:" << endl;
  for(int i = 0; i < 11; i++) cout << lose_points[i] << " ";
  cout << endl;
  cout << "decision_points : " << endl;
  for(int i = 0; i < 4; i++) cout << decision_points[i] << " ";
  cout << endl;
  cout << "End DS. " << endl;
  return;
}

int main(int, char *argv[]) {
  int a, b, c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);

  cout << "open : " << a << " " << b << " " << c << endl;

  int open[3] = {a, b, c};
  cfr_org(open);

  return 0;
}
