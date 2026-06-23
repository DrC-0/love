#include <map>
#include<numeric>
#include<random>

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "loveletter.hpp"


using namespace std;

bool br_switch = false;
int br_player = 0;
bool org_switch = false;
map<std::string, infset> table_infset{};
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;
static Rnd_Perfect_Hash rph;

#include "rnd_make_infset.hpp"
#include "bf_position.hpp"

void watch_cfr(int open[3]){
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

  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();++it){
    bf_position bfp(open, it->first);
    if(bfp.hand0[1] == 0){
        cout << "0";
        if(!it->second.wizard)
            bfp.print();
    }
  }
}

int main(int argc, char *argv[]){
  int a,b,c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);

  int open[3] = {a, b, c};
  cout << "open : " << open[0] << " " << open[1] << " " << open[2] << endl;

  watch_cfr(open);

  return 0;
}