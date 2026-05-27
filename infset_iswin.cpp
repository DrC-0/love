#include<iostream>
#include<fstream>
#include<map>
#include<cstdlib>
#include<random>
#include<string>
#include<numeric>
#include<algorithm>
#include<set>
#include<cassert>

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

std::set<std::string> only_history;
std::map<std::string, bool> abs_history;
int win_cnt[11];
int lose_cnt[11];
int win_move[11];
int lose_move[11];
int action_cnt = 0;
std::string max_history = "";
int hist_max = 0;

#include "rnd_make_infset.hpp"
#include "bf_position.hpp"


void cnt_abs(int open[3], string history){
  bf_position bfp(open, history);
  auto res_win = is_win(bfp);

  if (res_win.first) {
    win_move[res_win.second]++;
    abs_history.insert({history, true});

    if (res_win.second > hist_max) {
      hist_max = res_win.second;
      max_history = history;
    }
  }else win_move[0]++;

  auto lose_actions = is_lose(bfp);
  int able_act = action_count(bfp.hand0)- lose_actions.size();
  lose_move[0] += able_act ;
  for(const auto& lose_action : lose_actions){
    lose_move[lose_action.second]++;
    if(able_act == 1) only_history.insert(history);

    if (lose_action.second == 9) {
      output_actions_history(history, true);
    }
    else {
      // 特定のアクションの先が必敗の場合（元コードのロジックを忠実に再現）
      string action;
      action = rph.get_action((unsigned char)history[0]);

      int first = char_to_action(action[0]) / 10;
      auto actions = able_actions(bfp, lose_action.first, first == 2);

      for (int act : actions) {
        string new_hist = history + string(1, action2char(act, true));

        // それぞれの new_hist を挿入
        abs_history.insert({new_hist, false});
      }
    }
  }
}

void infset_iswin(int open[3]){
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
    action_cnt += action_count(bfp.hand0);
    cnt_abs(open, it->first);
  }
  assert(std::accumulate(win_move, win_move + 11, 0) == table_infset.size());
  assert(std::accumulate(lose_move, lose_move + 11, 0) == action_cnt);
  cout << "infset size:" << table_infset.size() << endl;
  cout << "win move:" << endl;
  for(int i = 0; i < 11; i++) cout << win_move[i] << " ";
  cout << endl;
  cout << "action count: " << action_cnt << endl;
  cout << "lose move:" << endl;
  for(int i = 0; i < 11; i++) cout << lose_move[i] << " ";
  cout << endl;
  cout << "hist count: "  << abs_history.size() << endl;
  cout << "max win history: " << hist_max << "turn " << get_actions_history(max_history, true) << endl;

  // 必勝判定・必敗判定を導入した際にめぐる必要のあるinfsetの数をカウント
  int infset_cnt[4] = {0, 0, 0, 0};
  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();++it){
    bool rm_bywin = false; bool rm_bylose = false;
    // abs_historyに含まれていたら必勝あるいは必敗
    auto ut = abs_history.upper_bound(it->first);
    if (ut != abs_history.begin()) {
      auto ut_prev = std::prev(ut);
      if (it->first.starts_with(ut_prev->first)) {
        if(ut_prev->second) rm_bywin = true;
        else rm_bylose = true;
      }
    }

    // 必敗の行動があり、行動が一つ(以下)の場合
    if(!rm_bywin && !rm_bylose && only_history.contains(it->first)){
      rm_bylose = true;
    }

    infset_cnt[0]++;
    if(!rm_bywin) infset_cnt[1]++;
    if(!rm_bylose) infset_cnt[2]++;
    if(!rm_bywin && !rm_bylose) infset_cnt[3]++;
  }

  cout << "infset size by win/lose:" << endl;
  for(int i = 0; i < 4; i++) cout << infset_cnt[i] << " ";
  cout << endl;
}

int main(int argc, char *argv[]){
  int a,b,c;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);

  // int open[3] = {4,4,6};
  // int open[3] = {5,5,7};
  int open[3] = {a, b, c};
  cout << "open : " << open[0] << " " << open[1] << " " << open[2] << endl;

  infset_iswin(open);

  return 0;
}
