// comp: win が書いた abs/wininf<部分ゲーム>.bin / abs/loseinf<部分ゲーム>.bin と、
// cfr が書いた str<部分ゲーム>64.bin を突き合わせ、belief_state の必勝・必敗判定が
// CFR の平均戦略 σ̄ と大きく食い違っていないかを調べる。
//
// 読み取り専用の調査ツール。既存のコードの挙動は変えない。rnd 側だけを見る
// (情報集合表は rnd のものしか無いため)。反復回数は 64 固定。
// 終了コードは常に 0 (調査ツールであり、CI の判定に使うものではないため)。
//
// win と同じ作業ディレクトリから走らせること (abs/wininf<部分ゲーム>.bin /
// abs/loseinf<部分ゲーム>.bin と str<部分ゲーム>64.bin を読む)。

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
#include <sstream>
#include <iomanip>
#include <vector>

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

#include "rnd_make_infset.hpp"
#include "belief_state_history.hpp"

// 純粋戦略からどれだけ離れていてよいか。|slot - σ̄| をこの値と比べる。
// 既定を 1e-2 にしてあるのは実測から。
// 64 反復の σ̄ は 0 や 1 に張り付かず、分布の = 0 / <1e-6 / <1e-4 のバケットは
// 446 / 557 とも 1 件も入らない。1e-6 にすると構造上 0 件しか出ない。
// 4 つ目の引数で上書きできる。分布も併せて出るので、それを見て決め直せる。
double eps = 1e-2;
constexpr int EXAMPLE_N = 10;

// 履歴を人が読める形にする。get_actions_history が使えない場合は
// 生のバイト列を16進で出す (履歴は unsigned char 列なのでそのままでは出せない)。
string display_history(const string& history) {
  try {
    return get_actions_history(history, true);
  } catch(...) {
    ostringstream oss;
    oss << hex << setfill('0');
    for(unsigned char c : history) oss << setw(2) << (int)c << ' ';
    return oss.str();
  }
}

// 行の slot を CFR の行動番号 (0/1) に直す。
// カード使用ノードはそのまま、魔術師の対象選択ノードは反転する
// (winlose_record.slot は 0 = 自分、CFR の行動0 は相手を対象とするため)。
int cfr_action(const winlose_record& row) {
  return row.is_choice_node ? (1 - row.slot) : row.slot;
}

// row が指すスロットの σ̄ を返す。sigma0 は「履歴 -> σ̄(行動0)」。
double sigma_for_slot(const winlose_record& row, const map<string, double>& sigma0) {
  auto it = sigma0.find(row.history);
  if(it == sigma0.end()) {
    fprintf(stderr, "comp: 行の履歴が σ̄ の表に無い (history length %zu)\n", row.history.size());
    abort();
  }
  return cfr_action(row) == 0 ? it->second : 1.0 - it->second;
}

// row の is_choice_node が、履歴から組み直した belief_state の is_wiz_choice と
// 一致するか確かめる。食い違ったらメッセージを出して false を返す (呼び出し側は
// その行を飛ばす)。
bool check_choice_node(int open[3], const winlose_record& row) {
  belief_state bs(open, row.history);
  if(row.is_choice_node != bs.is_wiz_choice) {
    fprintf(stderr,
            "comp: is_choice_node の食い違い (is_win=%d is_choice_node=%d bs.is_wiz_choice=%d, history=%s)\n",
            row.is_win, row.is_choice_node, bs.is_wiz_choice, display_history(row.history).c_str());
    return false;
  }
  return true;
}

// 分布のバケット番号。0 と 1 の両端を同じ細かさで見られるよう対称にしてある。
// 「ほぼ捨てている」(<1e-2) と「ほぼ決め打ち」(>=0.99) の両方を数えたいため。
constexpr int BUCKET_N = 8;
int bucket_index(double v) {
  if(v == 0.0) return 0;
  if(v < 1e-2) return 1;
  if(v < 0.1) return 2;
  if(v < 0.5) return 3;
  if(v < 0.9) return 4;
  if(v < 0.99) return 5;
  if(v < 1.0) return 6;
  return 7;
}

const char* const bucket_label[BUCKET_N] = {"=0", "<1e-2", "<0.1", "<0.5", "<0.9", "<0.99", "<1", "=1"};

void print_distribution(const char* title, const long long cnt[BUCKET_N]) {
  cout << title << ":";
  for(int i = 0; i < BUCKET_N; i++) {
    cout << "  " << bucket_label[i] << " " << cnt[i];
  }
  cout << endl;
}

int main(int argc, char* argv[]) {
  if(argc != 4 && argc != 5) {
    cerr << "usage: " << argv[0] << " <open1> <open2> <open3> [eps]" << endl;
    cerr << "  win と同じ作業ディレクトリから走らせること"
            " (abs/wininf<部分ゲーム>.bin と abs/loseinf<部分ゲーム>.bin を読む)。"
         << endl;
    cerr << "  反復回数は 64 固定。str<部分ゲーム>64.bin を読む。" << endl;
    cerr << "  eps は純粋戦略からの許容乖離。|slot - σ̄| と比べる。既定 1e-2。" << endl;
    return 0;
  }
  if(argc == 5) eps = atof(argv[4]);

  int open[3] = {atoi(argv[1]), atoi(argv[2]), atoi(argv[3])};
  cout << "comp " << open[0] << " " << open[1] << " " << open[2] << endl;

  string subgame = to_string(open[0] * 100 + open[1] * 10 + open[2]);

  // --- 情報集合表の構築 (infset_iswin.cpp:152-163 と同じ手順) ---
  node n_rnd_ds(open);
  rand_points++;
  for(int i = 1; i < 9; i++) {
    if(n_rnd_ds.deck[i - 1] == 0) continue;
    work_do_action ds_w;
    n_rnd_ds.do_action(1, i, ds_w);
    rnd_ds_put_hide_card(n_rnd_ds);
    n_rnd_ds.undo_action(1, i, ds_w);
  }

  // --- str<部分ゲーム>64.bin のサイズ検査 ---
  string str_filename = "str" + subgame + "64.bin";
  ifstream str_in(str_filename, ios::binary);
  if(!str_in) {
    cerr << "comp: str ファイルを開けなかった (" << str_filename << ")" << endl;
    return 0;
  }
  str_in.seekg(0, ios::end);
  const streamoff file_size = str_in.tellg();
  str_in.seekg(0, ios::beg);
  const size_t expected_size = 4 * sizeof(float) * table_infset.size();
  if(file_size < 0 || (size_t)file_size != expected_size) {
    cerr << "comp: " << str_filename << " のサイズが情報集合数と合わない"
         << " (ファイル " << file_size << " バイト、期待 " << expected_size
         << " バイト = 4*" << sizeof(float) << "*" << table_infset.size() << ")" << endl;
    cerr << "  部分ゲームと str ファイルの取り違えの可能性がある。" << endl;
    return 0;
  }

  // --- σ̄ の読み込み (cfr.cpp:85-97 と同じ形式) ---
  map<string, double> sigma0; // 履歴 -> σ̄(行動0)
  for(auto it = table_infset.begin(); it != table_infset.end(); ++it) {
    float sum_i0, sum_i1, regret0, regret1;
    str_in.read((char*)&sum_i0, sizeof(float));
    str_in.read((char*)&sum_i1, sizeof(float));
    str_in.read((char*)&regret0, sizeof(float));
    str_in.read((char*)&regret1, sizeof(float));
    (void)regret0;
    (void)regret1;
    double p = (sum_i1 == 0.0f) ? 0.5 : (double)sum_i0 / (double)sum_i1;
    sigma0.emplace(it->first, p);
  }

  // --- 行の読み込み ---
  vector<winlose_record> win_rows, lose_rows;
  load_bin_winlose("wininf" + subgame + ".bin", win_rows);
  load_bin_winlose("loseinf" + subgame + ".bin", lose_rows);

  cout << "  " << str_filename << "  (" << table_infset.size() << " 情報集合)" << endl;
  cout << "  abs/wininf" << subgame << ".bin  (" << win_rows.size() << " 行)" << endl;
  cout << "  abs/loseinf" << subgame << ".bin  (" << lose_rows.size() << " 行)" << endl;
  cout << "  EPS = " << eps << "   例示件数 N = " << EXAMPLE_N << endl;

  // --- is_choice_node の裏取り。食い違った行は以降の集計から除く ---
  vector<winlose_record> valid_win_rows, valid_lose_rows;
  for(const auto& row : win_rows) {
    if(check_choice_node(open, row)) valid_win_rows.push_back(row);
  }
  for(const auto& row : lose_rows) {
    if(check_choice_node(open, row)) valid_lose_rows.push_back(row);
  }

  // 同じ履歴に両方のスロットの行があるかどうか (履歴ごとの行数)。
  map<string, int> win_hist_count, lose_hist_count;
  set<string> win_hist_set, lose_hist_set;
  for(const auto& row : valid_win_rows) {
    win_hist_count[row.history]++;
    win_hist_set.insert(row.history);
  }
  for(const auto& row : valid_lose_rows) {
    lose_hist_count[row.history]++;
    lose_hist_set.insert(row.history);
  }

  // --- σ̄ の分布 (記録のある行のスロット側) ---
  // 両方のスロットに行がある履歴は除く。両方とも必敗なら σ̄ はどちらかに 0.5 以上を
  // 置かざるを得ず、両方とも必勝ならどちらに寄っていてもおかしくないので、
  // 入れると分布が読めなくなる。方向1 の除外と同じ規則。
  long long win_dist[BUCKET_N] = {};
  long long lose_dist[BUCKET_N] = {};
  long long win_both = 0, lose_both = 0;
  for(const auto& row : valid_win_rows) {
    if(win_hist_count[row.history] > 1) {
      win_both++;
      continue;
    }
    win_dist[bucket_index(sigma_for_slot(row, sigma0))]++;
  }
  for(const auto& row : valid_lose_rows) {
    if(lose_hist_count[row.history] > 1) {
      lose_both++;
      continue;
    }
    lose_dist[bucket_index(sigma_for_slot(row, sigma0))]++;
  }

  cout << endl
       << "--- P(スロットの行動) の分布 (両スロットに行がある履歴は除く) ---" << endl;
  print_distribution("必勝の行", win_dist);
  cout << "  (両スロットとも必勝で除いた行: " << win_both << ")" << endl;
  print_distribution("必敗の行", lose_dist);
  cout << "  (両スロットとも必敗で除いた行: " << lose_both << ")" << endl;

  // --- 方向1: 記録のある (履歴, スロット) の σ̄ が期待と食い違う ---
  long long win_violation = 0;
  long long lose_violation = 0;
  // 「この閾値では何も拾えない」のか「本当に一致している」のかを区別するため、
  // 検出しうる最小の閾値 (= 見た中で最も期待から外れていた値) を控えておく。
  double win_closest = 1.0; // 必勝の行で最も小さかった σ̄
  double lose_closest = 0.0; // 必敗の行で最も大きかった σ̄
  vector<string> win_examples, lose_examples;
  for(const auto& row : valid_win_rows) {
    if(win_hist_count[row.history] > 1) continue; // 両方のスロットに行があるなら数えない
    double s = sigma_for_slot(row, sigma0);
    win_closest = std::min(win_closest, s);
    if(s < 1.0 - eps) {
      win_violation++;
      if((int)win_examples.size() < EXAMPLE_N) {
        ostringstream oss;
        oss << "  " << display_history(row.history) << " slot=" << (int)row.slot
            << " choice=" << (row.is_choice_node ? 1 : 0) << " P(スロット)=" << s;
        win_examples.push_back(oss.str());
      }
    }
  }
  for(const auto& row : valid_lose_rows) {
    if(lose_hist_count[row.history] > 1) continue; // どちらも負けるので数えない
    double s = sigma_for_slot(row, sigma0);
    lose_closest = std::max(lose_closest, s);
    if(s > eps) {
      lose_violation++;
      if((int)lose_examples.size() < EXAMPLE_N) {
        ostringstream oss;
        oss << "  " << display_history(row.history) << " slot=" << (int)row.slot
            << " choice=" << (row.is_choice_node ? 1 : 0) << " P(スロット)=" << s;
        lose_examples.push_back(oss.str());
      }
    }
  }

  cout << endl
       << "--- 方向1: 記録と σ̄ が食い違う ---" << endl;
  cout << "必勝なのにスロットに寄り切っていない (P < 1-EPS) : " << win_violation;
  if(win_violation == 0) cout << "   (検出には EPS < " << 1.0 - win_closest << " が要る)";
  cout << endl;
  for(const auto& line : win_examples) cout << line << endl;
  cout << "必敗なのにスロットを捨て切っていない (P > EPS)   : " << lose_violation;
  if(lose_violation == 0) cout << "   (検出には EPS < " << lose_closest << " が要る)";
  cout << endl;
  for(const auto& line : lose_examples) cout << line << endl;

  // --- 方向2: σ̄ が偏っているのに記録が無い ---
  // 「どれくらい片方に寄っているか」は min(σ̄, 1 - σ̄) で測る。0 なら完全に
  // 片方だけ、0.5 なら五分。方向1 と同じバケットに入れる。
  long long no_record_total = 0;
  long long no_record_extreme = 0;
  long long no_record_dist[BUCKET_N] = {};
  double no_record_closest = 0.5; // 最も偏っていた min(σ̄, 1-σ̄)
  vector<string> no_record_examples;
  for(const auto& kv : sigma0) {
    const string& history = kv.first;
    if(win_hist_set.count(history) || lose_hist_set.count(history)) continue;
    no_record_total++;
    double p = kv.second;
    const double skew = std::min(p, 1.0 - p);
    no_record_dist[bucket_index(skew)]++;
    no_record_closest = std::min(no_record_closest, skew);
    if(p < eps || p > 1.0 - eps) {
      no_record_extreme++;
      if((int)no_record_examples.size() < EXAMPLE_N) {
        ostringstream oss;
        oss << "  " << display_history(history) << " P(行動0)=" << p;
        no_record_examples.push_back(oss.str());
      }
    }
  }

  cout << endl
       << "--- 方向2: σ̄ が偏っているのに記録が無い ---" << endl;
  cout << "記録の無い情報集合            : " << no_record_total << endl;
  cout << "  うち偏りが EPS 未満           : " << no_record_extreme;
  if(no_record_extreme == 0) cout << "   (検出には EPS > " << no_record_closest << " が要る)";
  cout << endl;
  for(const auto& line : no_record_examples) cout << line << endl;
  print_distribution("  偏りの分布 min(P(行動0), P(行動1))", no_record_dist);

  return 0;
}
