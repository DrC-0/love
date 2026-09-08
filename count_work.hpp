#ifndef COUNT_WORK_HPP
#define COUNT_WORK_HPP

// 判定の作業量を数える決定的カウンタ。
//
// 実行時間ではなく呼び出し回数で高速化を判定するための道具。同じ入力なら
// 常に同じ値が出るので、熱によるクロック低下やコード配置の違いに影響されない。
// gprof (-pg) を使わない理由は docs/adr/0003-no-gprof-for-performance.md を参照。
//
// -DCOUNT_WORK を付けたときだけ有効。付けなければ CW_BUMP は消えるので
// 通常ビルドの速度は変わらない。カウンタは標準エラーに出るため、本体の
// 標準出力は -DCOUNT_WORK の有無で変わらない。

#ifdef COUNT_WORK

#include <cstdio>

struct count_work_counters {
  // 判定関数
  unsigned long long is_terminated_win = 0;
  unsigned long long enemy_turn_win = 0;
  unsigned long long draw_win = 0;
  unsigned long long sol_win = 0;
  unsigned long long use_win = 0;
  unsigned long long use_lose = 0;
  unsigned long long wiz_win = 0;
  unsigned long long wiz_lose = 0;
  unsigned long long is_lose = 0;
  unsigned long long ef_wizard = 0;
  unsigned long long ef_wizard_elem = 0; // ef_wizard が返した要素の総数
  unsigned long long reset_flag_by_use = 0;
  unsigned long long bfp_from_history = 0; // 履歴文字列からの再構築

  // 8要素ループを回す派生量。回数がそのままループの実行回数になる。
  unsigned long long open_e = 0;
  unsigned long long count_deck = 0;
  unsigned long long deck = 0;
  unsigned long long hand_e_max = 0;
  unsigned long long hand_e_min = 0;
  unsigned long long deck_or_hand_e_min = 0;

  // ループの本体。呼び出し回数の合計が総反復回数になる。
  unsigned long long hand_e = 0;
  unsigned long long deck_or_hand_e = 0;

  ~count_work_counters() {
    std::fprintf(stderr,
                 "COUNT_WORK is_terminated_win=%llu enemy_turn_win=%llu draw_win=%llu "
                 "sol_win=%llu use_win=%llu use_lose=%llu\n"
                 "COUNT_WORK wiz_win=%llu wiz_lose=%llu is_lose=%llu ef_wizard=%llu "
                 "ef_wizard_elem=%llu reset_flag_by_use=%llu bfp_from_history=%llu\n"
                 "COUNT_WORK open_e=%llu count_deck=%llu deck=%llu hand_e_max=%llu "
                 "hand_e_min=%llu deck_or_hand_e_min=%llu\n"
                 "COUNT_WORK hand_e=%llu deck_or_hand_e=%llu\n",
                 is_terminated_win, enemy_turn_win, draw_win, sol_win, use_win, use_lose,
                 wiz_win, wiz_lose, is_lose, ef_wizard, ef_wizard_elem, reset_flag_by_use,
                 bfp_from_history,
                 open_e, count_deck, deck, hand_e_max, hand_e_min, deck_or_hand_e_min,
                 hand_e, deck_or_hand_e);
  }
};

inline count_work_counters cw_counters;

#define CW_BUMP(field) (++cw_counters.field)
#define CW_ADD(field, n) (cw_counters.field += (n))

#else

#define CW_BUMP(field) ((void)0)
#define CW_ADD(field, n) ((void)0)

#endif

#endif
