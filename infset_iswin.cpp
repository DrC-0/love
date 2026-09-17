#include <iostream>
#include <fstream>
#include <map>
#include <cstdlib>
#include <random>
#include <string>
#include <numeric>
#include <algorithm>
#include <set>
#include <cassert>

#include "rnd_action_sequense.hpp"
#include "rnd_action.hpp"
#include "org_action_sequense.hpp"
#include "loveletter.hpp"
#include "save_load_winlose.hpp"
#include "action_code.hpp"
#include "run_mode.hpp"
#include "analysis_points.hpp"

using namespace std;

bool br_switch = false;
int br_player = 0;
bool org_switch = false;
map<std::string, infset> table_infset{};
unsigned long int p1_points = 0;
unsigned long int p2_points = 0;
unsigned long int rand_points = 0;
unsigned long int end_points = 0;

std::set<std::string> only_history;
std::map<std::string, bool> abs_history;
std::vector<winlose_record> win_rows;
std::vector<winlose_record> lose_rows;
int win_cnt[11];
int lose_cnt[11];
int win_move[11];
int lose_move[11];
int action_cnt = 0;
std::string max_history = "";
int hist_max = 0;

#include "rnd_make_infset.hpp"
#include "belief_state_history.hpp"
#include "belief_state_lose.hpp"

void cnt_abs(int open[3], string history,
             belief_state_win_checker& wc, belief_state_lose_checker& lc) {
  belief_state bs(open, history);
  const win_decision res_win = wc.is_win(bs);

  if(res_win.has_win()) {
    // 「どれか勝てるか」は has_win()、「最短の手数」は立っているビットの turns
    // の最小値 (belief_state_win.hpp の is_win のコメント参照)。has_win() が真
    // なので少なくとも1本は立っており、turns は必ず初期値の 0 から更新される。
    int turns = 0;
    bool turns_set = false;
    for(int a = 0; a < 8; a++) {
      if(res_win.wins_with(a)) {
        turns = turns_set ? std::min(turns, (int)res_win.turns[a]) : (int)res_win.turns[a];
        turns_set = true;
      }
    }
    win_move[turns]++;
    abs_history.insert({history, true});

    if(turns > hist_max) {
      hist_max = turns;
      max_history = history;
    }

    // 終局判定を最初に見る。is_win も先頭でこれを呼び、決着していれば他の分岐を
    // 通らずに返す (belief_state_win.hpp の is_win_uncached)。
    // ここに来るのは has_win() が真のときだけなので、決着していれば won。
    // won は山札が尽きての勝ちで手札は1枚なので、行動はスロット0のカードしかない。
    if(check_terminal(bs) == terminal_kind::won) {
      win_rows.push_back({history, true, false, 0});
    } else if(!bs.hand_s[1].has_value() && bs.is_sol_choice) {
      // rnd では兵士の宣言は自然手番なので、宣言ノードが情報集合表に入ることは
      // 無いはず。通ったら行動を特定できないので、誤った行を書く代わりに落とす。
      winlose_unreachable(history, "sol_choice");
    } else if(!bs.hand_s[1].has_value() && bs.is_wiz_choice) {
      // 0 = 自分 (true)、1 = 相手 (false)。
      // is_lose の wiz 分岐 (belief_state_lose.hpp:44-48) と同じ対応。
      if(wc.wizard_win(bs, true).is_win) win_rows.push_back({history, true, true, 0});
      if(wc.wizard_win(bs, false).is_win) win_rows.push_back({history, true, true, 1});
    } else if(bs.is_my_turn && bs.hand_s[1].has_value()) {
      if(bs.hand_s[0] == bs.hand_s[1]) {
        // is_win と同じく片方だけ評価する。行動は 1 つしかない。
        if(wc.use_win(bs, bs.hand_s[0].value()).is_win)
          win_rows.push_back({history, true, false, 0});
      } else {
        if(wc.use_win(bs, bs.hand_s[0].value()).is_win)
          win_rows.push_back({history, true, false, 0});
        if(wc.use_win(bs, bs.hand_s[1].value()).is_win)
          win_rows.push_back({history, true, false, 1});
      }
    } else {
      // is_win の分岐をすべてなぞった上で残るものは無いはず。
      winlose_unreachable(history, "no_branch");
    }
  } else win_move[0]++;

  auto lose_actions = lc.is_lose(bs);
  int act_cnt = action_count(bs);
  int able_act = act_cnt - lose_actions.size();
  lose_move[0] += able_act;
  if(able_act == 1 && act_cnt > 1) only_history.insert(history);

  bool pushed[2] = {false, false}; // 同じスロットを 2 回積まないための印

  for(const auto& lose_action : lose_actions) {
    lose_move[lose_action.second]++;

    if(lose_action.first == 9) {
      output_actions_history(history, true);
    } else {
      // 特定のアクションの先が必敗の場合（元コードのロジックを忠実に再現）
      string action = rph.get_action((unsigned char)history[0]);

      int firstp = char_to_action(action[0]) / 10;
      auto actions = able_actions(bs, lose_action.first, firstp == 2);

      // able_actions が空なら今も abs_history に何も入らないので、行も書かない
      if(!actions.empty()) {
        bool is_choice_node;
        int slot;
        if(bs.is_wiz_choice) {
          // lose_action.first は 0 = 自分 / 1 = 相手。カードではない
          is_choice_node = true;
          slot = lose_action.first;
        } else {
          is_choice_node = false;
          slot = (bs.hand_s[0].has_value() && bs.hand_s[0].value().value() == lose_action.first)
                     ? 0
                     : 1;
        }
        if(!pushed[slot]) {
          pushed[slot] = true;
          lose_rows.push_back({history, false, is_choice_node, (unsigned char)slot});
        }
      }

      for(int act : actions) {
        string new_hist;
        if(bs.is_wiz_choice) {
          new_hist = history.substr(0, history.length() - 1) + string(1, action2char(act, true));
        } else {
          new_hist = history + string(1, action2char(act, true));
        }

        // それぞれの new_hist を挿入
        abs_history.insert({new_hist, false});
      }
    }
  }
}

void infset_iswin(int open[3]) {
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

  belief_state_win_checker wc;
  belief_state_lose_checker lc{wc};
  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
    belief_state bs(open, it->first);
    action_cnt += action_count(bs);
    cnt_abs(open, it->first, wc, lc);
  }

  assert(std::accumulate(win_move, win_move + 11, 0) == table_infset.size());
  assert(std::accumulate(lose_move, lose_move + 11, 0) == action_cnt);

  cout << "win move:" << endl;
  for(int i = 0; i < 11; i++) cout << win_move[i] << " ";
  cout << endl;
  cout << "lose move:" << endl;
  for(int i = 0; i < 11; i++) cout << lose_move[i] << " ";
  cout << endl;
  // cout << "hist count: "  << abs_history.size() << endl;
  // cout << "max win history: " << hist_max << "turn " << get_actions_history(max_history, true) << endl;

  // 必勝判定・必敗判定を導入した際にめぐる必要のあるinfsetの数をカウント
  int infset_cnt[4] = {0, 0, 0, 0};
  for(map<string, infset>::iterator it = table_infset.begin(); it != table_infset.end(); ++it) {
    bool rm_bywin = false;
    bool rm_bylose = false;
    // abs_historyに含まれていたら必勝あるいは必敗
    auto ut = abs_history.upper_bound(it->first);
    if(ut != abs_history.begin()) {
      auto ut_prev = std::prev(ut);
      if(it->first.starts_with(ut_prev->first)) {
        if(ut_prev->second) rm_bywin = true;
        else rm_bylose = true;
      }
    }
    // 必敗の行動があり、行動が一つ(以下)の場合
    if(!rm_bywin && !rm_bylose && only_history.contains(it->first)) {
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
  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);
  save_bin_winlose("wininf" + subgame + ".bin", win_rows);
  save_bin_winlose("loseinf" + subgame + ".bin", lose_rows);
}

int main(int, char* argv[]) {
  int a, b, c;
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
