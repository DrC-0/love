#ifndef ANALYSIS_POINTS_HPP
#define ANALYSIS_POINTS_HPP

// 解析中に数える統計カウンタ。定義は各 main の .cpp に残す。
// 詳細は docs/adr/0004-shared-declarations-headers.md を参照。
extern unsigned long int p1_points;
extern unsigned long int p2_points;
extern unsigned long int rand_points;
extern unsigned long int end_points;
extern unsigned long int win_points[11];
extern unsigned long int lose_points[11];
extern unsigned long int decision_points[4];
extern unsigned long int opengame;
extern unsigned long int soldior_points;
extern unsigned long int soldior_infsets;

#endif
