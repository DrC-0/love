#ifndef RUN_MODE_HPP
#define RUN_MODE_HPP

// 実行モードの切替。初期値はプログラムごとに違うため、定義は各 main の .cpp に残す。
// 詳細は docs/adr/0004-shared-declarations-headers.md を参照。
extern int cfr_switch;
extern int cfr_player;
extern bool br_switch;
extern int br_player;
extern bool org_switch;

#endif
