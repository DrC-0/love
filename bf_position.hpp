#ifndef MAX_NUM
const int max_num[8] = {5, 2, 2, 2, 2, 1, 1, 1};
#define MAX_NUM
#endif
#ifndef BF_POSITION_HPP
#define BF_POSITION_HPP
#include<string>
#include<vector>
#include<iostream>
#include<set>

#include "rnd_action.hpp"
#include "org_action.hpp"
#include "endgame.hpp"

using namespace std;

extern Rnd_Perfect_Hash rph;
extern Org_Perfect_Hash oph;
static std::set<std::string> win_history;
static std::set<std::string> lose_history;

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);
extern char action_to_char(int action, int card);
extern char wizard_to_char(int to, int trash, int draw);
extern char twonum_to_char(int card1, int card2);


struct bf_position{//14byte
  bool turn;
  bool not7_flag_s;
  bool not7_flag_e;
  bool barrier0;
  bool barrier1;
  bool lt5_flag_s;
  bool lt5_flag_e;
  int open_flag_s;
  int open_flag_e;
  int sol_flag_s;
  int sol_flag_e;
  int hand0[2];
  int trash[8];
  bf_position()
    : turn(false), not7_flag_s(false), not7_flag_e(false), barrier0(false), barrier1(false),
      lt5_flag_s(false), lt5_flag_e(false), open_flag_s(0), open_flag_e(0),
      sol_flag_s(0), sol_flag_e(0), hand0 {0, 0}, trash {0, 0, 0, 0, 0, 0, 0, 0} {}
  bf_position(bool turn, bool not7_flag_s, bool not7_flag_e, bool barrier0, bool barrier1,
              bool lt5_flag_s, bool lt5_flag_e, int open_flag_s, int open_flag_e,
              int sol_flag_s, int sol_flag_e, const int hand0[2], const int trash[8])
    : turn(turn),
      not7_flag_s(not7_flag_s),
      not7_flag_e(not7_flag_e),
      barrier0(barrier0),
      barrier1(barrier1),
      lt5_flag_s(lt5_flag_s),
      lt5_flag_e(lt5_flag_e),
      open_flag_s(open_flag_s),
      open_flag_e(open_flag_e),
      sol_flag_s(sol_flag_s),
      sol_flag_e(sol_flag_e){
        std::copy(hand0, hand0 + 2, this->hand0);
        std::copy(trash, trash + 8, this->trash);
      }
  bf_position(int open[3], std::string history, bool rnd);
  int deck_or_hand1(int i) const;
  bool hand1(int i) const;
  int open1() const;
  bool deck(int i) const;
  bool have0(int card) const;
  int other_hand0(int card) const;
  int count_deck() const;
  int hand_e_max() const;
  int hand_e_min() const;
  void print() const;
} ;

bool is_win(const bf_position& bfp);
int is_terminated_win(const bf_position& bfp);
bool use_win(const bf_position& bfp, int card);
bool draw_win(const bf_position& bfp);
bool enum_turn_win(const bf_position& bfp);
std::vector<bf_position> ef_wizard_win(const bf_position& bfp, bool to_0p);
bf_position draw(const bf_position& bfp, int draw_card);
bool rnd_is_win(int open[3], string history, bool rnd);
std::string actions_to_string(const std::vector<int> actions, bool rnd);
std::string get_actions_history(std::string s, bool rnd);
void output_actions_history(std::string s, bool rnd);
int is_terminated_lose(const bf_position& bfp);
bool is_lose(const bf_position& bfp);
bool use_lose(const bf_position& bfp, int card);

// int trash_and_hand_s(const int i, const int hand[2], const int trash[8]){
//   return trash[i] + (hand[0]  == i + 1 ? 1 : 0) + (hand[1]  == i + 1 ? 1 : 0);
// }

int deck_or_hand_e(const int i, const int hand[2], const int trash[8]){
  return max_num[i] - trash_and_hand_s(i, hand, trash);
}

bool hand_e(const int i, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e, const bool lt5_flag_e, const bool not7_flag){
  if(open_flag_e > 0){//手札が確定している場合
    if(open_flag_e == i + 1){
      return true;
    }else{
      return false;
    }
  }else if(sol_flag_e > 1 && sol_flag_e == i + 1){
    return false;
  }else if(lt5_flag_e && i + 1 >= 5){//前のターンに7を出したときの,5以上
    return false;
  }else if(not7_flag && i + 1 == 7){//前のターンに5を出したときの,7
    return false;
  }else{
    return deck_or_hand_e(i, hand, trash) > 0;
  }
}

int open_e(const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e, const bool lt5_flag_e, const bool not7_flag){
  int card = 0;
  for (int i = 0; i < 8; i++){
    if(hand_e(i, hand, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag)){
      if(card == 0){
        card = i+1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

bool deck(const int i, const int hand[2], const int trash[8], const int open_flag_e, const int sol_flag_e, const bool lt5_flag_e, const bool not7_flag){
  int open_card = open_e(hand, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag);
  return deck_or_hand_e(i, hand, trash) > (i + 1 == open_card ? 1 : 0);
}

// bool in(const int hand[2], const int target) {
//   return hand[0] == target || hand[1] == target;
// }

// int other(const int hand[2], const int target) {
//   if (hand[0] == target) return hand[1];
//   else if (hand[1] == target) return hand[0];
//   return 0;
// }

int count_deck(const int i, const int hand[2], const int trash[8]){
  int count = 0;
  for(int i = 0; i < 8; i++){
    count += deck_or_hand_e(i, hand, trash);
  }
  return count -1;
}

bf_position::bf_position(int open[3], string history, bool rnd): turn(false), not7_flag_s(false), not7_flag_e(false),
      barrier0(false), barrier1(false), lt5_flag_s(false), lt5_flag_e(false), open_flag_s(0),
      open_flag_e(0), sol_flag_s(0), sol_flag_e(0), hand0 {0, 0}, trash {0, 0, 0, 0, 0, 0, 0, 0} {
  for(int i = 0; i < 3; i++){
    trash[open[i]-1] += 1;
  }
  long unsigned int head = 0;
  bool is_second_player = false;
  while(head < history.size()){
    string action;
    if(rnd){
      action = rph.get_action((unsigned char)history[head]);
    } else {
      action = oph.get_action((unsigned char)history[head]);
    }
    int c2a = char_to_action(action[0]); head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;

    if(num1 == 1){
      hand0[0] = num2 + 1;
      turn = false;
      is_second_player = false;
    }
    else if(num1 == 2){
      hand0[0] = num2 + 1;
      turn = true;
      is_second_player = true;
    }
    else if(num1 == 3){
      hand0[1] = num2 + 1;
      barrier0 = false;
    }
    else if(num1 == 4){
      if(!turn){//player1のカード使用
        trash[num2] += 1;

        if(hand0[0] == num2 + 1){//手札を減らす
          hand0[0] = hand0[1];
          hand0[1] = 0;
        } else if (hand0[1] == num2 + 1){
          hand0[1] = 0;
        }

        //手札の候補のリセット
        barrier0 = false;
        if(open_flag_s > 0 && open_flag_s == num2 + 1){
          open_flag_s = 0;
        }
        if(sol_flag_s != 0 && sol_flag_s != num2 + 1){
          sol_flag_s = 0;
        }
        if(lt5_flag_s && num2 + 1 < 5){
          lt5_flag_s = false;
        }
        not7_flag_s = false;//対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

        if(!rnd && num2 + 1 == 1 && !barrier1){//ランダム宣言でない場合のみ
          int c2t = char_to_twonum(action[1]);
          sol_flag_e = (c2t % 10) + 1;
        }else if(num2 + 1 == 2 && !barrier1){
          int c2t = char_to_twonum(action[1]);
          open_flag_e = (c2t % 10) + 1;
        } else if(num2 + 1 == 3 && !barrier1){
          int c2t = char_to_twonum(action[1]);
          open_flag_e = (c2t % 10) + 1;
        } else if(num2 + 1 == 4){
          barrier0 = true;
        } else if(num2 + 1 == 5){
          int c2w = char_to_wizard(action[1]);
          int to = c2w / 100;
          int trashcard = (c2w / 10) % 10;
          int draw = c2w % 10;
          if(is_second_player == to){
            trash[trashcard] += 1;
            hand0[0] = draw + 1;
          } else if(!barrier1){
            trash[trashcard] += 1;
          }
        } else if(num2 + 1 == 6 && !barrier1){
          int c2t = char_to_twonum(action[1]);
          hand0[0] = (c2t % 10) + 1;
          open_flag_e = (c2t / 10) + 1;
        } else{
        }
      }
      else{//player2のカード使用
        trash[num2] += 1;

        //手札の候補のリセット
        barrier1 = false;
        if(open_flag_e > 0 && open_flag_e == num2 + 1){
          open_flag_e = 0;
        }
        if(sol_flag_e != 0 && sol_flag_e != num2 + 1){
          sol_flag_e = 0;
        }
        if(lt5_flag_e && num2 + 1 < 5){
          lt5_flag_e = false;
        }
        not7_flag_e = false;//対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

        if(num2 + 1 == 3 && !barrier0){
          int c2t = char_to_twonum(action[1]);
          open_flag_e = (c2t % 10) + 1;
        }
        if(num2 + 1 == 4){
          barrier1 = true;
        }
        if(num2 + 1 == 5 && !barrier0){
          not7_flag_e = true;
          int c2w = char_to_wizard(action[1]);
          int to = c2w / 100;
          int trashcard = (c2w / 10) % 10;
          int draw = c2w % 10;
          if(is_second_player != to){
            trash[trashcard] += 1;
          } else if(!barrier0){
            trash[trashcard] += 1;
            hand0[0] = draw + 1;
          }
        }
        if(num2 + 1 == 6 && !barrier0){
          int c2t = char_to_twonum(action[1]);
          hand0[0] = (c2t % 10) + 1;
          open_flag_e = (c2t / 10) + 1;
        }
        if(num2 + 1 == 7){
          lt5_flag_e = true;
        }
      }
      turn = !turn;
    }
  }
}

int bf_position::deck_or_hand1(int i) const {
  return deck_or_hand_e(i, hand0, trash);
}

bool bf_position::hand1(int i) const {
  return hand_e(i, hand0, trash, open_flag_e, sol_flag_e, lt5_flag_e, not7_flag_e);
}

int bf_position::open1() const {
  int card = 0;
  for (int i = 0; i < 8; i++){
    if(hand1(i)){
      if(card == 0){
        card = i+1;
      } else {
        return 0;
      }
    }
  }
  return card;
}

bool bf_position::deck(int i) const {
  int open_card = open1();
  return deck_or_hand1(i) > (i + 1 == open_card ? 1 : 0);
}

bool bf_position::have0(int card) const {
  return hand0[0] == card || hand0[1] == card;
}

int bf_position::other_hand0(int card) const {
  if(hand0[0] == card){
    return hand0[1];
  } else if(hand0[1] == card){
    return hand0[0];
  } else {
    return 0;
  }
}

int bf_position::count_deck() const {
  int count = 0;
  for(int i = 0; i < 8; i++){
    count += deck_or_hand1(i);
  }
  return count -1;
}

int bf_position::hand_e_max() const {
  int max_card = 0;
  for (int i = 0; i < 8; i++) {
    if (hand1(i)) {
      max_card = i + 1;
    }
  }
  return max_card;
}

int bf_position::hand_e_min() const {
  int min_card = 0;
  for (int i = 0; i < 8; i++) {
    if (hand1(i)) {
      if (min_card == 0 || i + 1 < min_card) {
        min_card = i + 1;
      }
    }
  }
  return min_card;
}


int is_terminated_win(const bf_position& bfp){
  if(bfp.have0(7) && bfp.hand0[0] + bfp.hand0[1] >= 12){
    return -1;
  }
  if(bfp.count_deck() < 2 && bfp.hand0[1] == 0){
    int max = bfp.hand_e_max();
    if(max == 0) exit(1);
    else if(max < bfp.hand0[0]) return 1;
    else return -1;
  }
  if(!bfp.barrier1 && bfp.hand0[1] > 0){
    if(bfp.have0(1) && bfp.open1() > 1){
      return 1;
    }
    if(bfp.have0(3) && bfp.hand_e_max() < bfp.other_hand0(3)){
      return 1;
    }
    if(bfp.have0(5) && bfp.open1() == 8){
      return 1;
    }
    if(bfp.have0(3) && bfp.hand_e_min() > bfp.other_hand0(3)){
      return -1;
    }
  }
  return 0;
}

bool is_win(const bf_position& bfp){
  int t = is_terminated_win(bfp);
  if(t == 1){
    return true;
  } else if(t == -1){
    return false;
  } else if(bfp.turn == 0){
    if(bfp.hand0[1] == 0){
      return draw_win(bfp);
    }else{
      if(bfp.hand0[0] == bfp.hand0[1]){
        return use_win(bfp, bfp.hand0[0]);
      } else {
        bf_position next_bfp = bfp;
        return use_win(next_bfp, next_bfp.hand0[0]) || use_win(bfp, bfp.hand0[1]);
      }
    }
  } else {
    return enum_turn_win(bfp);
  }
}

bool use_win(const bf_position& bfp, int card){
  int t = is_terminated_win(bfp);
  if(t == 1){
    return true;
  } else if(t == -1){
    return false;
  }
  struct bf_position next_bfp = bfp;
  next_bfp.turn = !bfp.turn;
  next_bfp.trash[card-1] += 1;//公開する
  // cout << count_deck() << "use :" << card << endl;
  //手札を減らす
  if(bfp.hand0[0] == card){
    next_bfp.hand0[0] = bfp.hand0[1];
    next_bfp.hand0[1] = 0;
  } else if(bfp.hand0[1] == card){
    next_bfp.hand0[1] = 0;
  }else {
    exit(1);
  }

  if(bfp.open_flag_s > 0 && bfp.open_flag_s == card){
    next_bfp.open_flag_s = 0;
  }
  if(bfp.sol_flag_s != 0 && bfp.sol_flag_s != card){
    next_bfp.sol_flag_s = 0;
  }
  if(bfp.lt5_flag_s && card < 5){
    next_bfp.lt5_flag_s = false;
  }
  // next_bfp.not7_flag = false;//対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

  if(card >= 5) next_bfp.not7_flag_s = true;

  if(!bfp.barrier1 && card != 4 && card != 5 && card != 7){
    return enum_turn_win(next_bfp);
  }
  else if(card == 1){
    for(int i = 1; i < 8; i++){
      if(bfp.hand1(i)){
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.sol_flag_e = i + 1;
        if(enum_turn_win(next_bfp2)){
          return true;
        }
      }
    }
    return false;
  }else if(card == 2){
    for(int i = 0; i < 8; i++){
      if(bfp.hand1(i)){
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.open_flag_e = i + 1;
        if(!enum_turn_win(next_bfp2)){
          return false;
        }
      }
    }
    return true;
  }else if(card == 3){
    for(int i = 0; i < 8; i++){
      if(bfp.hand1(i)){
        if(bfp.hand0[0] == i + 1){
          struct bf_position next_bfp2 = next_bfp;
          next_bfp2.open_flag_e = i + 1;
          if(!enum_turn_win(next_bfp2)){
            return false;
          }
        }else if(next_bfp.hand0[0] < i + 1){
          return false;
        }
      }
    }
    return true;
  }else if(card == 4){
    next_bfp.barrier0 = true;
    return enum_turn_win(next_bfp);
  }else if(card == 5){
    std::vector<bf_position> preds_toself = ef_wizard_win(next_bfp, true);
    if (preds_toself.empty()) return false;
    if (std::all_of(preds_toself.begin(), preds_toself.end(), [](const bf_position& p) {
      return enum_turn_win(p);
    })) {
      return true;
    }
    std::vector<bf_position> preds_toenemy = ef_wizard_win(next_bfp, false);
    if (std::all_of(preds_toenemy.begin(), preds_toenemy.end(), [](const bf_position& p) {
      return enum_turn_win(p);
    })) {
      return true;
    }
    return false;
    // struct bf_position next_bfp2 = next_bfp;
    // return ef_wizard_win(next_bfp2, true) || (bfp.barrier1 && enum_turn_win(next_bfp)) ||(!bfp.barrier1 && ef_wizard_win(next_bfp, false));
  }else if(card == 6){
    next_bfp.open_flag_e = bfp.other_hand0(card);
    for(int i = 0; i < 8; i++){
      if(bfp.hand1(i)){
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.hand0[0] = i + 1;
        if(!enum_turn_win(next_bfp2)){
          return false;
        }
      }
    }
    return true;
  }else if(card == 7){
    next_bfp.lt5_flag_s = true;
    return enum_turn_win(next_bfp);
  }else{
    return false;
  }
}

bool enum_turn_win(const bf_position& bfp){
  int t = is_terminated_win(bfp);
  if(t == 1){
    return true;
  } else if(t == -1){
    return false;
  }
  for(int i = 0; i < 7; i++){
    if(bfp.deck_or_hand1(i) > 0){
      // cout << count_deck() << "enum:" << i + 1 << endl;
      if(i + 1 == 1 && !bfp.barrier0 && bfp.hand0[0] > 1){
        return false;
      }
      if(i + 1 == 3 && !bfp.barrier0){
        for(int j = 0; j < 8; j++){
          if(bfp.deck_or_hand1(j)){
            if(j + 1 == 3 && bfp.deck_or_hand1(2) >= 2 && bfp.hand0[0] < 3){//相手が3を2枚持っている可能性がある場合
              return false;
            }
            else if(j + 1 > bfp.hand0[0]){//自分の手札より強いカードが存在する場合
              return false;
            }
          }
        }
      }
      if(i + 1 == 5 && bfp.hand0[0] == 8 && !bfp.barrier0){//相手が魔術師を持っていて自分が姫を持っている場合
        return false;
      }
      struct bf_position next_bfp = bfp;
      next_bfp.trash[i] += 1;

      next_bfp.barrier1 = false;
      next_bfp.turn = !bfp.turn;
      if(bfp.open_flag_e > 0 && bfp.open_flag_e == i + 1){
        next_bfp.open_flag_e = 0;
      }
      if(bfp.sol_flag_e != 0 && bfp.sol_flag_e != i + 1){
        next_bfp.sol_flag_e = 0;
      }
      if(bfp.lt5_flag_e && i + 1 < 5){
        next_bfp.lt5_flag_e = false;
      }
      next_bfp.not7_flag_e = false;//対象が7しかないため次のターンに7を出す出さないに関わらず推理がリセット

      if( i + 1 >= 5) next_bfp.not7_flag_e = true;
      if( i + 1 == 3){
        if(!bfp.barrier0){//騎士を使って残りのカードが手札以下の強さの場合(!=未満)
          next_bfp.open_flag_e = bfp.hand0[0];
        }
        if(!draw_win(next_bfp)){
          return false;
        }
      } else if( i + 1 == 4){
        next_bfp.barrier1 = true;
        if(!draw_win(next_bfp)){
          return false;
        }
      } else if( i + 1 == 5){
        if(bfp.open1() == 7){
          continue;
        }
        std::vector<bf_position> preds_toself = ef_wizard_win(next_bfp, true);
        if (!std::all_of(preds_toself.begin(), preds_toself.end(), [](const bf_position& p) {
          return enum_turn_win(p);
        })) {
          return false;
        }
        std::vector<bf_position> preds_toenemy = ef_wizard_win(next_bfp, false);
        if (preds_toself.empty()) return false;
        if (!std::all_of(preds_toenemy.begin(), preds_toenemy.end(), [](const bf_position& p) {
          return enum_turn_win(p);
        })) {
          return false;
        }
        // struct bf_position next_bfp2 = next_bfp;
        // if(!ef_wizard_win(next_bfp2, false) || !(bfp.barrier0 && draw_win(next_bfp)) ||!(!bfp.barrier0 && ef_wizard_win(next_bfp, true))){
        //   return false;
        // }
      } else if( i + 1 == 6){
        if(bfp.open1() == 7){
          continue;
        }
        if(!bfp.barrier0){
          for(int j = 0; j < 8; j++){
            if(next_bfp.hand1(j)){
              struct bf_position next_bfp2 = next_bfp;
              next_bfp2.hand0[0] = j + 1;
              next_bfp2.open_flag_e = bfp.hand0[0];
              if(!draw_win(next_bfp2)){
                return false;
              }
            }
          }
        } else {
          if(!draw_win(next_bfp)){
            return false;
          }
        }
      } else if( i + 1 == 7){
        int open_card = bfp.open1();
        if(open_card >= 5 && open_card != 7){
          continue;
        }
        next_bfp.lt5_flag_e = true;
        if(!draw_win(next_bfp)){
          return false;
        }
      } else if(!draw_win(next_bfp)){
        return false;
      }
    }
  }
  return true;
}

bool draw_win(const bf_position& bfp){
  int t = is_terminated_win(bfp);
  if(t == 1){
    return true;
  } else if(t == -1){
    return false;
  }
  for(int i = 0; i < 8; i++){
    if(bfp.deck(i)){
      // cout << count_deck() << "draw:" << i + 1 << endl;
      bf_position bfp_h0 = draw(bfp, i + 1);
      bfp_h0.barrier0 = false;
      bfp_h0.turn = !bfp.turn;
      bf_position bfp_h1 = bfp_h0;
      if(!(use_win(bfp_h0, bfp_h0.hand0[0]) || (bfp.hand0[0] != i + 1 && use_win(bfp_h1, bfp_h1.hand0[1])))){
        return false;
      }
    }
  }
  return true;
}

bf_position draw(const bf_position& bfp, int draw_card){
  assert(bfp.deck(draw_card - 1) && bfp.count_deck() > 0 && bfp.hand0[1] == 0);
  bf_position next_bfp = bfp;
  next_bfp.hand0[1] = draw_card;
  return next_bfp;
}

std::vector<bf_position> ef_wizard_win(const bf_position& bfp, bool to_0p){
  std::vector<bf_position> bfps;
  if(bfp.turn == 1){//use_abswinの最初でturnを切り替えるためturn==1は0playerのターン
    if(to_0p){
      if(bfp.hand0[0] == 8){
        // return false;
        return bfps;
      }
      for(int i = 0; i < 8; i++){
        if(bfp.deck(i)){
          bf_position next_bfp = bfp;
          next_bfp.turn = !bfp.turn;
          next_bfp.trash[bfp.hand0[0]-1] += 1;//手札捨てる
          next_bfp.hand0[0] = i + 1;//手札引く
          bfps.push_back(next_bfp);
          // if(!enum_turn_win(next_bfp)){
          //   return false;
          // }
        }
      }
      return bfps;
      // return true;
    } else {
      if(bfp.barrier1){
        bfps.push_back(bfp);
        return bfps;
      }
      for(int i = 0; i < 8; i++){
        if(bfp.hand1(i) && i + 1 != 8){
          bf_position next_bfp = bfp;
          next_bfp.trash[i] += 1;
          next_bfp.turn = !bfp.turn;
          bfps.push_back(next_bfp);
          // if(!enum_turn_win(next_bfp)){
          //   return false;
          // }
        }
      }
      // return true;
      return bfps;
    }
  } else {//turn == 0
    if(to_0p){
      if(bfp.barrier0){
        bfps.push_back(bfp);
        return bfps;
      }
      for(int i = 0; i < 8; i++){
        if(bfp.deck(i)){
          bf_position next_bfp = bfp;
          next_bfp.turn = !bfp.turn;
          next_bfp.trash[bfp.hand0[0]-1] += 1;//手札捨てる
          next_bfp.hand0[0] = i + 1;//手札引く
          bfps.push_back(next_bfp);
          // if(!draw_win(next_bfp)){
          //   return false;
          // }
        }
      }
      return bfps;
      // return true;
    } else {
      if(bfp.open1() == 8){
        // return true;
        return bfps;
      }
      for(int i = 0; i < 8; i++){
        if(bfp.hand1(i) && i + 1 != 8){
          bf_position next_bfp = bfp;
          next_bfp.turn = !bfp.turn;
          next_bfp.trash[i] += 1;
          bfps.push_back(next_bfp);
          // if(!draw_win(next_bfp)){
          //   return false;
          // }
        }
      }
      // return true;
      return bfps;
    }
  }
}

void bf_position::print() const{
  // cout << "depth : " << depth << endl;
  cout << "barrier0 : " << barrier0 << " barrier1 : " << barrier1 << " turn : " << turn << endl;
  cout << "open_flag_e : " << open_flag_e << " sol_flag_e : " << sol_flag_e << " lt5_flag_e : " << lt5_flag_e << " not7_flag_e : " << not7_flag_e << endl;
  cout << "hand0 : " << hand0[0] << " " << hand0[1] << " ";
  cout << "trash:";
  for(int i = 0; i < 8; i++){
    cout << trash[i] << " ";
  }
  cout << " deck_or_hand1 : ";
  for(int i = 0; i < 8; i++){
    cout << deck_or_hand1(i) << " ";
  }
  cout << endl;
  cout << "hand1: ";
  for(int i = 0; i < 8; i++){
    cout << hand1(i) << " ";
  }
  cout << endl << endl;
}

bool rnd_is_win(int open[3], string history, bool rnd){
  bf_position bfp(open, history, true);
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
    return true;
  } else if(is_win(bfp)){
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
  return false;
}

std::string actions_to_string(const std::vector<int> actions, bool rnd){
  string history;
  for(auto x : actions){
    int act1,act2;
    if(x > 10000){
      act1 = x / 1000;
      act2 = x % 1000;
    }
    else if(x > 1000){
      act1 = x / 100;
      act2 = x % 100;
    } else if (x > 100){
      act1 = x / 10;
      act2 = x % 10;
    } else {
      act1 = x;
      act2 = 0;
    }
    char action[2];
    size_t s = 1;
    action[0] = action_to_char(act1 / 10, act1 % 10);
    if(act2 > 0){
      if(act1 % 10 == 4){
        action[1] = wizard_to_char(act2/100, (act2/10)%10, act2%10);
      } else if (act1 % 10 == 2){
        action[1] = twonum_to_char(act2%10, act2%10);
      }
      else{
        action[1] = twonum_to_char(act2/10, act2%10);
      }
      s = 2;
    }

    unsigned int h;
    if(rnd){
      h = rph.get_hash(action, s);
    } else {
      h = oph.get_hash(action, s);
    }
    history.push_back(static_cast<unsigned char>(h));
  }
  return history;
}

string get_actions_history(std::string s, bool rnd){
  long unsigned int head = 0;
  int turn = 0;
  bool bal[2] = {false, false};
  string history = "";
  while(head < s.size()){
    string action;
    if(rnd){
      action = rph.get_action((unsigned char)s[head]);
    } else {
      action = oph.get_action((unsigned char)s[head]);
    }
    int c2a = char_to_action(action[0]); head++;
    int num1 = c2a / 10;
    int num2 = c2a % 10;
    history += to_string(c2a);
    // cout << c2a;
    bal[turn] = false;
    if(num1 == 4){
      if( num2 == 0 && bal[1-turn]){
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t%10);
        // cout << (c2t % 10);
      } else if(num2 == 1 && !bal[1-turn]){
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t%10);
        // cout << (c2t % 10);
      } else if(num2 == 2 && !bal[1-turn]){
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t%10);
        // cout << (c2t % 10);
      } else if(num2 == 3){
        bal[turn] = true;
      } else if(num2 == 4){
        int c2w = char_to_wizard(action[1]);
        history += to_string(c2w/100);
        history += to_string((c2w/10)%10);
        history += to_string(c2w%10);
        // cout << c2w / 100 << ((c2w / 10) % 10) << (c2w % 10);
        // cout << c2w;
      } else if(num2 == 5 && !bal[1-turn]){
        int c2t = char_to_twonum(action[1]);
        history += to_string(c2t/10);
        history += to_string(c2t%10);
        // cout << (c2t / 10) << (c2t % 10);
      }
      turn = !turn;
    }
    // cout << " ";
    history += " ";
  }
  // cout << endl;
  return history;
}

void output_actions_history(std::string s, bool rnd){
  cout << get_actions_history(s, rnd) << endl;
}

int is_terminated_lose(const bf_position& bfp){
  if(bfp.have0(7) && bfp.hand0[0] + bfp.hand0[1] >= 12){
    return -1;
  }
  if(bfp.count_deck() < 2 && bfp.hand0[1] == 0){
    int min = bfp.hand_e_min();
    if(min == 0) exit(1);
    else if(min > bfp.hand0[0]) return -1;
    else return 1;
  }
  if(!bfp.barrier1 && bfp.hand0[1] > 0){
    if(bfp.have0(1) && bfp.open1() > 1){
      return 1;
    }
    if(bfp.have0(3) && bfp.hand_e_max() < bfp.other_hand0(3)){
      return 1;
    }
    if(bfp.have0(5) && bfp.open1() == 8){
      return 1;
    }
    if(bfp.have0(3) && bfp.hand_e_min() > bfp.other_hand0(3)){
      return -1;
    }
  }
  return 0;
}

bool is_lose(const bf_position& bfp){
  if(bfp.turn == 1 || bfp.hand0[1] != 0) exit(1);
  int t = is_terminated_lose(bfp);
  if(t == 1){
    return false;
  } else if(t == -1){
    return true;
  }
  if(bfp.have0(8)) return true;
  return use_lose(bfp, bfp.hand0[0]) || use_lose(bfp, bfp.hand0[1]);
}

bool use_lose(const bf_position& bfp, int card){
  struct bf_position next_bfp = bfp;
  next_bfp.turn = !bfp.turn;
  next_bfp.trash[card-1] += 1;//公開する
  // cout << count_deck() << "use :" << card << endl;
  //手札を減らす
  if(bfp.hand0[0] == card){
    next_bfp.hand0[0] = bfp.hand0[1];
    next_bfp.hand0[1] = 0;
  } else if(bfp.hand0[1] == card){
    next_bfp.hand0[1] = 0;
  }else {
    exit(1);
  }

  if(bfp.open_flag_s > 0 && bfp.open_flag_s == card){
    next_bfp.open_flag_s = 0;
  }
  if(bfp.sol_flag_s != 0 && bfp.sol_flag_s != card){
    next_bfp.sol_flag_s = 0;
  }
  if(bfp.lt5_flag_s && card < 5){
    next_bfp.lt5_flag_s = false;
  }
  return false;
}

#endif