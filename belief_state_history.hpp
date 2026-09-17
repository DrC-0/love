#ifndef BELIEF_STATE_HISTORY_HPP
#define BELIEF_STATE_HISTORY_HPP
#include "belief_state.hpp"

belief_state::belief_state(int open[3], std::string history, bool rnd = true)
  : is_my_turn(false), is_sol_choice(false), is_wiz_choice(false),
    not7_flag_s(false), not7_flag_e(false), barrier_s(false), barrier_e(false), lt5_flag_s(false), lt5_flag_e(false), open_flag_s(),
    open_flag_e(), sol_flag_s{}, sol_flag_e{}, hand_s{}, trash{0, 0, 0, 0, 0, 0, 0, 0} {
  CW_BUMP(belief_state_from_history);
  for(int i = 0; i < 3; i++) {
    trash[open[i] - 1] += 1;
  }
  long unsigned int head = 0;
  bool is_second_player = false;
  while(head < history.size()) {
    std::string action;
    if(rnd) action = rph.get_action((unsigned char)history[head]);
    else action = oph.get_action((unsigned char)history[head]);
    int c2a = char_to_action(action[0]);
    head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;

    if(num1 == 1) {
      hand_s[0] = Card{num2 + 1};
      is_my_turn = true;
      is_second_player = false;
    } else if(num1 == 2) {
      hand_s[0] = Card{num2 + 1};
      is_my_turn = false;
      is_second_player = true;
    } else if(num1 == 3) {
      hand_s[1] = Card{num2 + 1};
      barrier_s = false;
    } else if(num1 == 4) {
      is_sol_choice = false;
      is_wiz_choice = false;
      if(is_my_turn) { // player1のカード使用
        trash[num2] += 1;

        if(hand_s[0] == Card{num2 + 1}) { //手札を減らす
          hand_s[0] = hand_s[1];
          hand_s[1] = MaybeCard();
        } else if(hand_s[1] == Card{num2 + 1}) {
          hand_s[1] = MaybeCard();
        }

        //手札の候補のリセット
        barrier_s = false;
        *this = reset_flag_by_use(*this, true, Card{num2 + 1});

        if(num2 + 1 == 1) {
          //相手が保護されている場合、宣言自体が行われないため選択ノードにはならない
          if(!barrier_e) {
            if(action.size() > 1) {
              int c2t = char_to_twonum(action[1]);
              int choice = (c2t % 10) + 1;
              if(choice > 1) add_sol_e(Card{choice});
            } else is_sol_choice = true;
          }
        } else if(num2 + 1 == 2 && !barrier_e) {
          int c2t = char_to_twonum(action[1]);
          open_flag_e = Card{(c2t % 10) + 1};
        } else if(num2 + 1 == 3 && !barrier_e) {
          int c2t = char_to_twonum(action[1]);
          open_flag_e = Card{(c2t % 10) + 1};
          open_flag_s = Card{(c2t % 10) + 1};
        } else if(num2 + 1 == 4) {
          barrier_s = true;
        } else if(num2 + 1 == 5) {
          if(action.size() > 1) {
            not7_flag_s = true;
            int c2w = char_to_wizard(action[1]);
            int to = c2w / 100;
            int trashcard = (c2w / 10) % 10;
            int draw = c2w % 10;
            if(is_second_player == to) {
              trash[trashcard] += 1;
              hand_s[0] = Card{draw + 1};
              reset_flag(true); //自分のフラグリセット
            } else if(!barrier_e) {
              trash[trashcard] += 1;
              reset_flag(false); //相手のフラグリセット
            }
          } else is_wiz_choice = true;
        } else if(num2 + 1 == 6 && !barrier_e) {
          int c2t = char_to_twonum(action[1]);
          hand_s[0] = Card{(c2t % 10) + 1};
          reset_flag(true);
          reset_flag(false);
          open_flag_s = Card{(c2t % 10) + 1};
          open_flag_e = Card{(c2t / 10) + 1};
        } else if(num2 + 1 == 7) {
          lt5_flag_s = true;
        } else {
        }
      } else { // player2のカード使用
        trash[num2] += 1;

        //手札の候補のリセット
        barrier_e = false;
        *this = reset_flag_by_use(*this, false, Card{num2 + 1});
        if(num2 + 1 == 1 && !barrier_s && action.size() > 1) {
          int c2t = char_to_twonum(action[1]);
          int choice = (c2t % 10) + 1;
          if(choice > 1) add_sol_s(Card{choice});
        } else if(num2 + 1 == 2 && !barrier_s) {
          int c2t = char_to_twonum(action[1]);
          open_flag_s = Card{(c2t % 10) + 1};
        } else if(num2 + 1 == 3 && !barrier_s) {
          int c2t = char_to_twonum(action[1]);
          open_flag_e = Card{(c2t % 10) + 1};
          open_flag_s = Card{(c2t % 10) + 1};
        } else if(num2 + 1 == 4) {
          barrier_e = true;
        } else if(num2 + 1 == 5) {
          not7_flag_e = true;
          int c2w = char_to_wizard(action[1]);
          int to = c2w / 100;
          int trashcard = (c2w / 10) % 10;
          int draw = c2w % 10;
          if(is_second_player != to) {
            trash[trashcard] += 1;
            reset_flag(false); //相手のフラグリセット
          } else if(!barrier_s) {
            trash[trashcard] += 1;
            hand_s[0] = Card{draw + 1};
            reset_flag(true); //自分のフラグリセット
          }
        } else if(num2 + 1 == 6 && !barrier_s) {
          int c2t = char_to_twonum(action[1]);
          hand_s[0] = Card{(c2t % 10) + 1};
          reset_flag(true);
          reset_flag(false);
          open_flag_s = Card{(c2t % 10) + 1};
          open_flag_e = Card{(c2t / 10) + 1};
        } else if(num2 + 1 == 7) {
          lt5_flag_e = true;
        }
      }
      is_my_turn = !is_my_turn;
    }
  }
  // 履歴が選択待ちノードで終わっているなら、その選択はカードを出した本人の
  // 意思決定なので手番はまだ渡っていない。ループ中の反転を戻す。
  // ここで戻さないと、履歴から復元した belief_state と use_win 経由で作った
  // belief_state で is_my_turn が逆になり、ef_wizard が別の枝に落ちる。
  // 途中で立つぶんは次の行動の先頭で false に戻るので、末尾だけを見ればよい
  // (rnd は兵士の宣言を抽象化しているため、途中で is_sol_choice が立つ)。
  if(is_sol_choice || is_wiz_choice) is_my_turn = true;
}

unsigned char action2char(int x, bool rnd);
std::string actions_to_string(const std::vector<int>& actions, bool rnd);
std::string get_actions_history(std::string s, bool rnd);
void output_actions_history(std::string s, bool rnd);

unsigned char action2char(int x, bool rnd) {
  int act1, act2;
  if(x > 10000) {
    act1 = x / 1000;
    act2 = x % 1000;
  } else if(x > 1000) {
    act1 = x / 100;
    act2 = x % 100;
  } else if(x > 100) {
    act1 = x / 10;
    act2 = x % 10;
  } else {
    act1 = x;
    act2 = 0;
  }

  char action[2];
  size_t s = 1;
  action[0] = action_to_char(act1 / 10, act1 % 10);

  if(act2 > 0) {
    if(act1 % 10 == 4) {
      action[1] = wizard_to_char(act2 / 100, (act2 / 10) % 10, act2 % 10);
    } else if(act1 % 10 == 2) {
      action[1] = twonum_to_char(act2 % 10, act2 % 10);
    } else {
      action[1] = twonum_to_char(act2 / 10, act2 % 10);
    }
    s = 2;
  }

  unsigned int h;
  if(rnd) {
    h = rph.get_hash(action, s);
  } else {
    h = oph.get_hash(action, s);
  }
  return static_cast<unsigned char>(h);
}

std::string actions_to_string(const std::vector<int>& actions, bool rnd) {
  std::string history;
  history.reserve(actions.size());

  for(auto x : actions) {
    history.push_back(action2char(x, rnd));
  }
  return history;
}

std::string get_actions_history(std::string s, bool rnd = true) {
  long unsigned int head = 0;
  int turn = 0;
  bool bal[2] = {false, false};
  std::string history = "";
  while(head < s.size()) {
    std::string action;
    if(rnd) {
      action = rph.get_action((unsigned char)s[head]);
    } else {
      action = oph.get_action((unsigned char)s[head]);
    }
    int c2a = char_to_action(action[0]);
    head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;
    history += std::to_string(c2a);
    // std::cout << c2a;
    bal[turn] = false;
    if(num1 == 4) {
      if(num2 == 0 && !bal[1 - turn] && action.size() > 1) {
        int c2t = char_to_twonum(action[1]);
        history += std::to_string(c2t % 10);
      } else if(num2 == 1 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        history += std::to_string(c2t % 10);
        // std::cout << (c2t % 10);
      } else if(num2 == 2 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        history += std::to_string(c2t % 10);
        // std::cout << (c2t % 10);
      } else if(num2 == 3) {
        // 次の行動で turn が反転するため bal[1 - turn] として読まれるが,
        // cppcheck は添字の変化を追えず未使用と誤検知する
        // cppcheck-suppress unreadVariable
        bal[turn] = true;
      } else if(num2 == 4) {
        int c2w = char_to_wizard(action[1]);
        history += std::to_string(c2w / 100);
        history += std::to_string((c2w / 10) % 10);
        history += std::to_string(c2w % 10);
        // std::cout << c2w / 100 << ((c2w / 10) % 10) << (c2w % 10);
        // std::cout << c2w;
      } else if(num2 == 5 && !bal[1 - turn]) {
        int c2t = char_to_twonum(action[1]);
        history += std::to_string(c2t / 10);
        history += std::to_string(c2t % 10);
        // std::cout << (c2t / 10) << (c2t % 10);
      }
      turn = !turn;
    }
    // std::cout << " ";
    history += " ";
  }
  // std::cout << std::endl;
  return history;
}

void output_actions_history(std::string s, bool rnd = true) {
  std::cout << get_actions_history(s, rnd) << std::endl;
}
#endif
