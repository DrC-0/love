#include<iostream>
#include<fstream>
#include<map>
#include<cstdlib>
#include<random>
#include<vector>
#include<string>
#include<numeric>
#include<algorithm>

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "loveletter.hpp"

extern int char_to_action(char c);

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
int infset_cnt[11];
int win_cnt[11];

#include "rnd_make_infset.hpp"
#include "bf_position.hpp"
#include "endgame.hpp"
// #include "bf_positionL.hpp"

State get_state_by_bfp(const bf_position& bfp) {
    State s;
    s.barrier = bfp.barrier0;
    s.not7_flag = bfp.not7_flag_e;
    s.lt5_flag_s = bfp.lt5_flag_s;
    s.lt5_flag_e = bfp.lt5_flag_e;
    s.open_flag_s = bfp.open_flag_s;
    s.open_flag_e = bfp.open_flag_e;
    s.sol_flag_s = bfp.sol_flag_s;
    s.sol_flag_e = bfp.sol_flag_e;
    if(bfp.hand0[0] >= bfp.hand0[1]){
      s.hand[0] = bfp.hand0[0];
      s.hand[1] = bfp.hand0[1];
    }else{
      s.hand[0] = bfp.hand0[1];
      s.hand[1] = bfp.hand0[0];
    }
    for(int i = 0; i < 8; i++){
      s.trash[i] = bfp.trash[i];
    }
    return s;
}

bool rnd_is_win_state(int open[3], string history, bool rnd){
  bf_position bfp(open, history, true);
  // bfp.print();
  State s = get_state_by_bfp(bfp);
  // std::cout << get_hash(s) << std::endl;
  // if(bfp.hand0[0] == 3 && bfp.hand0[1] == 1){
  //   output_actions_history(history, rnd);
  // }
  // s.print();
  bool is_prefix_match = false;
  std::string parent;
  auto it_ub = win_history.upper_bound(history);
  if (it_ub != win_history.begin()) {
    auto it_prev = std::prev(it_ub);
    if (history.rfind(*it_prev, 0) == 0) {
      is_prefix_match = true;
      parent = *it_prev;
    }
  }
  if (is_prefix_match){
    // std::cout << "true" << std::endl;
    return true;
  } else if(abs_win(s)){
    // std::cout << "true" << std::endl;
    win_history.insert(history);
    for (auto it_clean = it_ub; it_clean != win_history.end(); ) {
        if (it_clean->rfind(history, 0) == 0) {
            it_clean = win_history.erase(it_clean);
        } else {
            break;
        }
    }
    return true;
  }
  // std::cout << "false" << std::endl;
  return false;
}

void infset_iswin(int open[3], bool brp1){
    string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
    string filename1 = "str" + subgame + "64.bin";
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
        string action = rph.get_action((unsigned char)it->first[0]);
        int c2a = char_to_action(action[0]);
        int first = c2a / 10;
        if((first == 1 && brp1) || (first == 2 && !brp1)){
            it = table_infset.erase(it);
        } else {
            double ave_strategy = (double)sum_i0 / (double)sum_i1;
            it->second.set_prob_action(ave_strategy);
            ++it;
        }
    }
    cout << "End make_infset." << endl;

    // vector<string> check_history;
    for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();++it){
        bf_position bfp(open, it->first, true);
        int deck = bfp.count_deck();
        infset_cnt[deck]++;
        // if(rnd_is_win_state(open, it->first, true)){
        if(rnd_is_win(open, it->first, true)){
        // if(rnd_is_lose(open, it->first, true)){
            win_cnt[deck]++;
        }
    }
    cout << "infset count and win count by deck size:" << endl;
    for(int i = 0; i < 11; i++) cout << infset_cnt[i] << " ";
    cout << endl;
    for(int i = 0; i < 11; i++) cout << win_cnt[i] << " ";
    cout << endl;
    cout << open[0] << open[1] << open[2] << "_" << brp1 << ":" <<
     std::accumulate(std::begin(win_cnt), std::end(win_cnt), 0) << endl;
    // for (const auto& history : check_history) {
    //     cout << get_actions_history(history, true) << endl;
    // }
    // cout << "check history size: " << check_history.size() << endl;
    return;
}

int main(int argc, char *argv[]){
  int p;
//   int a,b,c;
  p = atoi(argv[1]);
//   a = atoi(argv[2]);
//   b = atoi(argv[3]);
//   c = atoi(argv[4]);

  bool brp1;
  if(p == 1){
    brp1 = true;
    cout << "P1 best responce." << endl;
  } else if(p == 2) {
    brp1 = false;
    cout << "P2 best responce." << endl;
  } else {
    terminate();
  }

  int open[3] = {5,5,7};
//   int open[3] = {a, b, c};
  cout << "open : " << open[0] << " " << open[1] << " " << open[2] << endl;

  infset_iswin(open, brp1);

  return 0;
}
