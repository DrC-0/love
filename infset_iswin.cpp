#include<iostream>
#include<fstream>
#include<map>
#include<cstdlib>
#include<random>
#include<vector>
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

int infset_cnt[11];
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

bool rnd_is_win(int open[3], string history, bool rnd){
  bf_position bfp(open, history, rnd);
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

bool rnd_is_lose(int open[3], string history, bool rnd){
  bf_position bfp(open, history, rnd);

  auto it_ub = lose_history.upper_bound(history);
  if (it_ub != lose_history.begin()) {
    auto it_prev = std::prev(it_ub);
    if (history.rfind(*it_prev, 0) == 0) {
      lose_move[0]++;
      return true; // プレフィックスが見つかったらここで早期リターン
    }
  }

  auto loseaction = is_lose(bfp);
  if(loseaction.first){
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
      if(rnd) action = rph.get_action((unsigned char)history[0]);
      else action = oph.get_action((unsigned char)history[0]);
      int first = char_to_action(action[0]) / 10;
      auto actions = able_actions(bfp, loseaction.first, rnd, first);
      for (int act : actions) {
        std::string new_hist = history + std::string(1, action2char(act, rnd));
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

bool rnd_abs(int open[3], string history, bool rnd){
  bf_position bfp(open, history, rnd);
  int deck = bfp.count_deck();
  auto it_ub = abs_history.upper_bound(history);
  auto it_search = it_ub;

  while (it_search != abs_history.begin()) {
    auto it_prev = std::prev(it_search);

    // 前方一致しているかチェック
    if (history.rfind(it_prev->first, 0) == 0) {
      // 一致した要素の second (bool) に応じてカウントを分岐
      if (it_prev->second) {
        win_move[0]++;
        win_cnt[deck]++;
      } else {
        lose_move[0]++;
        lose_cnt[deck]++;
      }
      return true; // 前方一致が見つかった時点で終了
    } else break;// 辞書順で手前が前方一致しなくなった時点で、これ以上遡っても存在しない
  }

  auto res_win = is_win(bfp);
  if (res_win.first) {
    // 新しい勝利履歴を abs_history に挿入 (値は true)
    auto it_inserted = abs_history.insert(it_ub, {history, true});

    win_cnt[deck]++;
    win_move[res_win.second]++;
    if (res_win.second > hist_max) {
      hist_max = res_win.second;
      max_history = history;
    }

    // 挿入した位置の「次の要素」から探索を開始
    auto it_clean = std::next(it_inserted);
    while (it_clean != abs_history.end()) {
      // 自分(history)が、相手(it_clean->first)の前方一致になっているか
      if (it_clean->first.rfind(history, 0) == 0)
        it_clean = abs_history.erase(it_clean);
      else break; // 辞書順で前方一致しなくなった時点でループを抜ける
    }
    return true;
  }

auto lose_action = is_lose(bfp);
  if (lose_action.first) {
    // ★元コードのカウンタ仕様に合わせて lose_cnt[deck] に修正
    lose_move[lose_action.second]++;
    lose_cnt[deck]++; // ★元コードの呼び出し側のインクリメントを補填

    if (lose_action.second == 9) {
      // 完全に history 自体が必敗の場合
      auto it_inserted = abs_history.insert(it_ub, {history, false});

      if (lose_action.second > hist_max) { // (必要であれば勝敗共通の最大履歴更新)
        hist_max = lose_action.second;
        max_history = history;
      }

      auto it_clean = std::next(it_inserted);
      while (it_clean != abs_history.end()) {
        if (it_clean->first.rfind(history, 0) == 0)
          it_clean = abs_history.erase(it_clean);
        else break;
      }
    }
    else {
      // 特定のアクションの先が必敗の場合（元コードのロジックを忠実に再現）
      string action;
      if(rnd) action = rph.get_action((unsigned char)history[0]);
      else action = oph.get_action((unsigned char)history[0]);

      int first = char_to_action(action[0]) / 10;
      auto actions = able_actions(bfp, lose_action.first, rnd, first);

      for (int act : actions) {
        std::string new_hist = history + std::string(1, action2char(act, rnd));

        // それぞれの new_hist を挿入
        auto it_ins_new = abs_history.insert({new_hist, false}).first;

        // それぞれの new_hist から始まる長い履歴をクリーンアップ
        auto it_clean = std::next(it_ins_new);
        while (it_clean != abs_history.end()) {
          if (it_clean->first.rfind(new_hist, 0) == 0) {
            it_clean = abs_history.erase(it_clean);
          } else {
            break;
          }
        }
      }
    }
    return true;
  }
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
    vector<string> check_history;
    for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end();++it){
      bf_position bfp(open, it->first, true);
      int deck = bfp.count_deck();
      infset_cnt[deck]++;
      // if(rnd_is_win(open, it->first, true)){
      //   win_cnt[deck]++;
      // }
      // else if(rnd_is_lose(open, it->first, true)){
      //   lose_cnt[deck]++;
      // }
      rnd_abs(open, it->first, true);
    }
    // cout << "max depth: " << max_depth << endl;
    cout << "infset count by deck size:" << endl;
    for(int i = 0; i < 11; i++) cout << infset_cnt[i] << " ";
    cout << endl;
    cout << "win count:" << endl;
    for(int i = 0; i < 11; i++) cout << win_cnt[i] << " ";
    cout << endl;
    cout << "lose count:" << endl;
    for(int i = 0; i < 11; i++) cout << lose_cnt[i] << " ";
    cout << endl;
    cout << "win move:" << endl;
    for(int i = 0; i < 11; i++) cout << win_move[i] << " ";
    cout << endl;
    cout << "lose move:" << endl;
    for(int i = 0; i < 11; i++) cout << lose_move[i] << " ";
    cout << endl;
    cout << "win: " << open[0] << open[1] << open[2] << "_" << brp1 << ":" <<
     std::accumulate(std::begin(win_cnt), std::end(win_cnt), 0) << endl;
    cout << "lose: " << open[0] << open[1] << open[2] << "_" << brp1 << ":" <<
     std::accumulate(std::begin(lose_cnt), std::end(lose_cnt), 0) << endl;
    // for (const auto& history : check_history) {
    //   cout << get_actions_history(history, true) << endl;
    // }
    // cout << "check history size: " << check_history.size() << endl;
    cout << "max win history: " << hist_max << "turn " << get_actions_history(max_history, true) << endl;
    return;
}

int main(int argc, char *argv[]){
  int p;
  p = atoi(argv[1]);

  int a,b,c;
  a = atoi(argv[2]);
  b = atoi(argv[3]);
  c = atoi(argv[4]);

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

  // int open[3] = {4,4,6};
  // int open[3] = {5,5,7};
  int open[3] = {a, b, c};
  cout << "open : " << open[0] << " " << open[1] << " " << open[2] << endl;

  infset_iswin(open, brp1);

  return 0;
}
