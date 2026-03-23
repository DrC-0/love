#include "bf_position.hpp"
static std::set<std::string> lose_history;

bool is_lose(const bf_position& bfp);
bool use_lose(const bf_position& bfp, int card);
bool draw_lose(const bf_position& bfp);
bool enum_turn_lose(const bf_position& bfp);
bool ef_wizard_lose(const bf_position& bfp, bool to_0p);
bool rnd_is_lose(int open[3], string history, bool rnd);

bool is_lose(const bf_position& bfp){
  int t = is_terminated_abswin(bfp);
  if(t == 1){
    return false; // 自分が勝つので必敗ではない
  } else if(t == -1){
    return true;  // 自分が負けるので必敗
  } else if(bfp.turn == 0){
    if(bfp.hand0[1] == 0){
      return draw_lose(bfp);
    }else{
      if(bfp.hand0[0] == bfp.hand0[1]){
        // 同一カードの場合は片方のみ判定。ただし必勝手は必敗手ではないので除外する
        return use_lose(bfp, bfp.hand0[0]) && !use_win(bfp, bfp.hand0[0]);
      } else {
        bf_position next_bfp = bfp;
        // どちらかのカードを使って負けるなら必敗 (OR条件)
        // ただし必勝手は必敗手ではないため、!use_win を条件に加える
        bool lose0 = use_lose(next_bfp, next_bfp.hand0[0]) && !use_win(next_bfp, next_bfp.hand0[0]);
        bool lose1 = use_lose(bfp, bfp.hand0[1]) && !use_win(bfp, bfp.hand0[1]);
        return lose0 || lose1;
      }
    }
  } else {
    return enum_turn_lose(bfp);
  }
}

bool use_lose(const bf_position& bfp, int card){
  int t = is_terminated_abswin(bfp);
  if(t == 1) return false;
  else if(t == -1) return true;

  struct bf_position next_bfp = bfp;
  next_bfp.turn = !bfp.turn;
  next_bfp.trash[card-1] += 1;

  if(bfp.hand0[0] == card){
    next_bfp.hand0[0] = bfp.hand0[1];
    next_bfp.hand0[1] = 0;
  } else {
    next_bfp.hand0[1] = 0;
  }

  if(card == 1){
    if(bfp.barrier1) return enum_turn_lose(next_bfp);
    if(next_bfp.open1() > 1) return false;
    // 自分がどの数字を宣言しても負けるか (負けを回避する手がないか = AND条件)
    for(int i = 1; i < 8; i++){
      if(next_bfp.hand1(i)){
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.sol_flag = i + 1;
        if(!enum_turn_lose(next_bfp2)){
          return false;
        }
      }
    }
    return true;
  }else if(card == 2){
    if(bfp.barrier1) return enum_turn_lose(next_bfp);
    for(int i = 0; i < 8; i++){
      if(next_bfp.hand1(i)){
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.open_flag = i + 1;
        if(!enum_turn_lose(next_bfp2)){
          return false;
        }
      }
    }
    return true;
  }else if(card == 3){
    if(bfp.barrier1) return enum_turn_lose(next_bfp);
    for(int i = 0; i < 8; i++){
      if(next_bfp.hand1(i)){
        if(next_bfp.hand0[0] == i + 1){
          struct bf_position next_bfp2 = next_bfp;
          next_bfp2.open_flag = i + 1;
          if(!enum_turn_lose(next_bfp2)){
            return false;
          }
        }else if(next_bfp.hand0[0] < i + 1){
          continue; // 自分が負ける
        }else{
          return false; // 自分が勝つので必敗ではない
        }
      }
    }
    return true;
  }else if(card == 4){
    next_bfp.barrier0 = true;
    return enum_turn_lose(next_bfp);
  }else if(card == 5){
    struct bf_position next_bfp2 = next_bfp;
    // どちらの対象を選んでも負けるか (回避不可か = AND条件)
    return ef_wizard_lose(next_bfp2, true) && (bfp.barrier1 ? enum_turn_lose(next_bfp) : ef_wizard_lose(next_bfp, false));
  }else if(card == 6){
    if(bfp.barrier1) return enum_turn_lose(next_bfp);
    next_bfp.open_flag = next_bfp.hand0[0];
    for(int i = 0; i < 8; i++){
      if(next_bfp.hand1(i)){
        struct bf_position next_bfp2 = next_bfp;
        next_bfp2.hand0[0] = i + 1;
        if(!enum_turn_lose(next_bfp2)){
          return false;
        }
      }
    }
    return true;
  }else if(card == 7){
    return enum_turn_lose(next_bfp);
  }else{
    return false;
  }
}

bool enum_turn_lose(const bf_position& bfp){
  int t = is_terminated_abswin(bfp);
  if(t == 1) return false;
  else if(t == -1) return true;

  for(int i = 0; i < 7; i++){
    if(bfp.deck_or_hand1(i) > 0){
      // 相手は自分(P0)を確実に負かせる手があればそれを選ぶ (OR条件)
      if(i + 1 == 1 && !bfp.barrier0 && bfp.hand0[0] > 1){
        return true;
      }
      if(i + 1 == 3 && !bfp.barrier0){
        for(int j = 0; j < 8; j++){
          if(bfp.deck_or_hand1(j)){
            if(j + 1 == 3 && bfp.deck_or_hand1(2) >= 2 && bfp.hand0[0] < 3){
              return true;
            }
            else if(j + 1 > bfp.hand0[0]){
              return true;
            }
          }
        }
      }
      if(i + 1 == 5 && bfp.hand0[0] == 8 && !bfp.barrier0){
        return true;
      }

      struct bf_position next_bfp = bfp;
      next_bfp.trash[i] += 1;

      next_bfp.barrier1 = false;
      next_bfp.turn = !bfp.turn;
      if(bfp.open_flag > 0 && bfp.open_flag == i + 1){
        next_bfp.open_flag = 0;
      }
      if(bfp.sol_flag != 0 && bfp.sol_flag != i + 1){
        next_bfp.sol_flag = 0;
      }
      if(bfp.lt5_flag && i + 1 < 5){
        next_bfp.lt5_flag = false;
      }
      next_bfp.not7_flag = false;

      if( i + 1 == 3){
        if(!bfp.barrier0) next_bfp.open_flag = bfp.hand0[0];
        if(draw_lose(next_bfp)) return true;
      } else if( i + 1 == 4){
        next_bfp.barrier1 = true;
        if(draw_lose(next_bfp)) return true;
      } else if( i + 1 == 5){
        if(bfp.open1() == 7) continue;
        struct bf_position next_bfp2 = next_bfp;
        // 相手は、相手自身が勝てる（P0を負かせる）対象を選ぶ (OR条件)
        if(ef_wizard_lose(next_bfp2, false) || (bfp.barrier0 && draw_lose(next_bfp)) || (!bfp.barrier0 && ef_wizard_lose(next_bfp, true))){
          return true;
        }
      } else if( i + 1 == 6){
        if(bfp.open1() == 7) continue;
        if(!bfp.barrier0){
          for(int j = 0; j < 8; j++){
            if(next_bfp.hand1(j)){
              struct bf_position next_bfp2 = next_bfp;
              next_bfp2.hand0[0] = j + 1;
              next_bfp2.open_flag = bfp.hand0[0];
              if(draw_lose(next_bfp2)){
                return true;
              }
            }
          }
        } else {
          if(draw_lose(next_bfp)) return true;
        }
      } else if( i + 1 == 7){
        int open_card = bfp.open1();
        if(open_card >= 5 && open_card != 7) continue;
        next_bfp.lt5_flag = true;
        if(draw_lose(next_bfp)) return true;
      } else {
        if(draw_lose(next_bfp)) return true;
      }
    }
  }
  return false;
}

bool draw_lose(const bf_position& bfp){
  int t = is_terminated_abswin(bfp);
  if(t == 1) return false;
  else if(t == -1) return true;

  for(int i = 0; i < 8; i++){
    if(bfp.deck(i)){
      bf_position bfp_h0 = draw(bfp, i + 1);
      bfp_h0.barrier0 = false;
      bfp_h0.turn = !bfp.turn;
      bf_position bfp_h1 = bfp_h0;

      // 未来の自分のターンでは、自分は最善を尽くして負けを回避しようとするため
      // 引いた後の両方のカードの選択肢が共に「負けルート」である場合のみ必敗となる (AND条件)
      // ただしここでも「必勝手」は「必敗手」として扱わない
      bool lose0 = use_lose(bfp_h0, bfp_h0.hand0[0]) && !use_win(bfp_h0, bfp_h0.hand0[0]);
      bool lose1 = use_lose(bfp_h1, bfp_h1.hand0[1]) && !use_win(bfp_h1, bfp_h1.hand0[1]);

      if(bfp.hand0[0] == i + 1){
         if(!lose0) return false;
      } else {
         if(!(lose0 && lose1)) return false;
      }
    }
  }
  return true;
}

bool ef_wizard_lose(const bf_position& bfp, bool to_0p){
  if(bfp.turn == 1){ // 自分が打つ場合
    if(to_0p){
      if(bfp.hand0[0] == 8) return true;
      for(int i = 0; i < 8; i++){
        if(bfp.deck(i)){
          bf_position next_bfp = bfp;
          next_bfp.turn = !bfp.turn;
          next_bfp.trash[bfp.hand0[0]-1] += 1;
          next_bfp.hand0[0] = i + 1;
          if(!enum_turn_lose(next_bfp)) return false;
        }
      }
      return true;
    } else {
      if(bfp.open1() == 8) return false;
      for(int i = 0; i < 8; i++){
        if(bfp.hand1(i) && i + 1 != 8){
          bf_position next_bfp = bfp;
          next_bfp.trash[i] += 1;
          next_bfp.turn = !bfp.turn;
          if(!enum_turn_lose(next_bfp)) return false;
        }
      }
      return true;
    }
  } else { // 相手が打つ場合
    if(to_0p){
      if(bfp.hand0[0] == 8) return true;
      for(int i = 0; i < 8; i++){
        if(bfp.deck(i)){
          bf_position next_bfp = bfp;
          next_bfp.turn = !bfp.turn;
          next_bfp.trash[bfp.hand0[0]-1] += 1;
          next_bfp.hand0[0] = i + 1;
          if(!draw_lose(next_bfp)) return false;
        }
      }
      return true;
    } else {
      if(bfp.open1() == 8) return false;
      for(int i = 0; i < 8; i++){
        if(bfp.hand1(i) && i + 1 != 8){
          bf_position next_bfp = bfp;
          next_bfp.turn = !bfp.turn;
          next_bfp.trash[i] += 1;
          if(!draw_lose(next_bfp)) return false;
        }
      }
      return true;
    }
  }
}

bool rnd_is_lose(int open[3], string history, bool rnd){
  bf_position bfp(open, history, true);
  bool is_prefix_match = false;
  std::string parent;
  auto it_ub = lose_history.upper_bound(history);
  if (it_ub != lose_history.begin()) {
    auto it_prev = std::prev(it_ub);
    if (history.rfind(*it_prev, 0) == 0) {
      is_prefix_match = true;
      parent = *it_prev;
    }
  }
  if (is_prefix_match){
    return true;
  } else if(is_lose(bfp)){
    lose_history.insert(history);
    for (auto it_clean = it_ub; it_clean != lose_history.end(); ) {
        if (it_clean->rfind(history, 0) == 0) {
            it_clean = lose_history.erase(it_clean);
        } else {
            break;
        }
    }
    return true;
  }
  return false;
}