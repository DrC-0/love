#include<iostream>
#include<fstream>
#include<map>
#include<cstdlib>
#include<random>
#include<string>
#include<numeric>
#include<algorithm>
#include<set>

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

std::set<std::string> win_history;
std::set<std::string> lose_history;
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
// #include "endgame.hpp"

// State get_state_by_bfp(const bf_position& bfp) {
//     State s;
//     s.barrier = bfp.barrier1;
//     s.not7_flag = bfp.not7_flag_e;
//     s.lt5_flag_s = bfp.lt5_flag_s;
//     s.lt5_flag_e = bfp.lt5_flag_e;
//     s.open_flag_s = bfp.open_flag_s;
//     s.open_flag_e = bfp.open_flag_e;
//     s.sol_flag_s = bfp.sol_flag_s;
//     s.sol_flag_e = bfp.sol_flag_e;
//     if(bfp.hand0[0] >= bfp.hand0[1]){
//       s.hand[0] = bfp.hand0[0];
//       s.hand[1] = bfp.hand0[1];
//     }else{
//       s.hand[0] = bfp.hand0[1];
//       s.hand[1] = bfp.hand0[0];
//     }
//     for(int i = 0; i < 8; i++){
//       s.trash[i] = bfp.trash[i];
//     }
//     return s;
// }

// bool rnd_is_win_state(int open[3], string history, bool rnd){
//   bf_position bfp(open, history, true);
//   // bfp.print();
//   if(bfp.hand0[1] == 0) return false;
//   State s = get_state_by_bfp(bfp);
//   // std::cout << get_hash(s) << std::endl;
//   // if(bfp.hand0[0] == 3 && bfp.hand0[1] == 1){
//   //   output_actions_history(history, rnd);
//   // }
//   // s.print();
//   bool is_prefix_match = false;
//   std::string parent;
//   auto it_ub = win_history.upper_bound(history);
//   if (it_ub != win_history.begin()) {
//     auto it_prev = std::prev(it_ub);
//     if (history.rfind(*it_prev, 0) == 0) {
//       is_prefix_match = true;
//       parent = *it_prev;
//     }
//   }
//   if (is_prefix_match){
//     // std::cout << "true" << std::endl;
//     return true;
//   } else if(abs_win(s)){
//     // std::cout << "true" << std::endl;
//     win_history.insert(history);
//     for (auto it_clean = it_ub; it_clean != win_history.end(); ) {
//       if (it_clean->rfind(history, 0) == 0) {
//         it_clean = win_history.erase(it_clean);
//       } else {
//         break;
//       }
//     }
//     return true;
//   }
//   // std::cout << "false" << std::endl;
//   return false;
// }

bool rnd_is_win(int open[3], string history){
  bf_position bfp(open, history);
  auto it_ub = win_history.upper_bound(history);
  if (it_ub != win_history.begin()) {
    auto it_prev = std::prev(it_ub);
    if (history.rfind(*it_prev, 0) == 0) {
      win_move[0]++;
      return true;
    }
  }
  auto res = is_win(bfp);
  if(res.first){
    win_history.insert(history);
    win_move[res.second]++;
    if(res.second > hist_max){
      hist_max = res.second;
      max_history = history;
    }
    for (auto it_clean = it_ub; it_clean != win_history.end(); ) {
        if (it_clean->rfind(history, 0) == 0) {
          it_clean = win_history.erase(it_clean);
        } else {
          break;
        }
    }
    return true;
  }
  return false;
}

bool rnd_is_lose(int open[3], string history){
  bf_position bfp(open, history);

  auto it_ub = lose_history.upper_bound(history);
  if (it_ub != lose_history.begin()) {
    auto it_prev = std::prev(it_ub);
    if (history.rfind(*it_prev, 0) == 0) {
      lose_move[0]++;
      return true; // プレフィックスが見つかったらここで早期リターン
    }
  }

  auto loseactions = is_lose(bfp);
  if(loseactions.empty()){
    lose_move[0]++;
    return true;
  }
  for(const auto& loseaction : loseactions){
    lose_move[loseaction.second]++;
    if(loseaction.second == 9) {
      lose_history.insert(history);

      // history 自体が必敗なので、history から始まる長い履歴をすべて削除
      auto it_clean = lose_history.upper_bound(history);
      while (it_clean != lose_history.end() && it_clean->rfind(history, 0) == 0) {
        it_clean = lose_history.erase(it_clean);
      }
    }
    else {
      string action;
      action = rph.get_action((unsigned char)history[0]);
      int first = char_to_action(action[0]) / 10;
      auto actions = able_actions(bfp, loseaction.first, first == 1);
      for (int act : actions) {
        std::string new_hist = history + std::string(1, action2char(act, true));
        lose_history.insert(new_hist);

        // 追加した new_hist から始まる長い履歴のみを個別に削除
        auto it_clean = lose_history.upper_bound(new_hist);
        while (it_clean != lose_history.end() && it_clean->rfind(new_hist, 0) == 0) {
          it_clean = lose_history.erase(it_clean);
        }
      }
    }
    return true;
  }

  return false;
}

void cnt_abs(int open[3], string history){
  bf_position bfp(open, history);

  auto res_win = is_win(bfp);
  if (res_win.first) {
    win_move[res_win.second]++;
    abs_history.insert({history, false});


    if (res_win.second > hist_max) {
      hist_max = res_win.second;
      max_history = history;
    }
  }else win_move[0]++;

  auto lose_actions = is_lose(bfp);
  int act = action_count(bfp.hand0);
  lose_move[0] += act - lose_actions.size();
  for(const auto& lose_action : lose_actions){
    lose_move[lose_action.second]++;

    if (lose_action.second == 9) {
      output_actions_history(history, true);
    }
    else {
      // 特定のアクションの先が必敗の場合（元コードのロジックを忠実に再現）
      string action;
      action = rph.get_action((unsigned char)history[0]);

      int first = char_to_action(action[0]) / 10;
      auto actions = able_actions(bfp, lose_action.first, first == 1);

      for (int act : actions) {
        std::string new_hist = history + std::string(1, action2char(act, true));

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
  return;
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
