#ifndef SAVE_LOAD_WINLOSE_HPP
#define SAVE_LOAD_WINLOSE_HPP
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

// 意味の違う値を同じ欄に入れると読む側が必ず取り違えるので、slot の読み方を
// 別の欄で示す (ADR 0008 で Card / MaybeCard を分けたのと同じ理由)。
struct winlose_record {
  std::string history; // rnd の情報集合の履歴
  bool is_win; // true = 必勝、false = 必敗
  bool is_choice_node; // true = 魔術師の対象選択ノード
  unsigned char slot; // 0 か 1。
                      // is_choice_node が偽なら手札スロット (0 = hand_s[0]、1 = hand_s[1])
                      // is_choice_node が真なら魔術師の対象 (0 = 自分、1 = 相手)
};

// rnd の情報集合表には現れないはずのノードに当たったときに落とす。
// 通常ビルドは -DNDEBUG で assert が無効なので、常時有効の検査として書く。
[[noreturn]] inline void winlose_unreachable(const std::string& history, const char* what) {
  std::fprintf(stderr,
               "winlose: rnd に無いはずの情報集合に当たった (%s), history length %zu\n",
               what, history.size());
  std::abort();
}

inline void save_bin_winlose(const std::string& filename,
                             const std::vector<winlose_record>& rows) {
  fs::path dir_path = "abs";
  if(!fs::exists(dir_path)) {
    fs::create_directories(dir_path);
  }
  fs::path file_path = dir_path / filename;

  std::ofstream ofs(file_path, std::ios::binary);
  if(!ofs) {
    std::fprintf(stderr, "save_bin_winlose: ファイルを開けませんでした (%s)\n", file_path.c_str());
    std::abort();
  }

  size_t row_count = rows.size();
  ofs.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));

  for(const auto& row : rows) {
    if(row.history.empty() || row.history.size() > 255) {
      std::fprintf(stderr,
                   "save_bin_winlose: history length out of range (%zu)\n",
                   row.history.size());
      std::abort();
    }
    uint8_t len = static_cast<uint8_t>(row.history.size());
    ofs.write(reinterpret_cast<const char*>(&len), sizeof(len));
    ofs.write(row.history.data(), len);

    uint8_t flags = 0;
    if(row.is_win) flags |= 0x1;
    if(row.slot) flags |= 0x2;
    if(row.is_choice_node) flags |= 0x4;
    ofs.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
  }

  if(!ofs) {
    std::fprintf(stderr, "save_bin_winlose: 書き込みに失敗した (%s)\n", file_path.c_str());
    std::abort();
  }
}

inline void load_bin_winlose(const std::string& filename,
                             std::vector<winlose_record>& rows) {
  fs::path file_path = fs::path("abs") / filename;

  std::ifstream ifs(file_path, std::ios::binary);
  if(!ifs) {
    std::fprintf(stderr, "load_bin_winlose: ファイルを開けませんでした (%s)\n", file_path.c_str());
    std::abort();
  }

  rows.clear();

  size_t row_count = 0;
  ifs.read(reinterpret_cast<char*>(&row_count), sizeof(row_count));
  if(!ifs) {
    std::fprintf(stderr, "load_bin_winlose: 行数を読めなかった (%s)\n", file_path.c_str());
    std::abort();
  }

  for(size_t i = 0; i < row_count; ++i) {
    uint8_t len = 0;
    ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
    if(!ifs) {
      std::fprintf(stderr, "load_bin_winlose: %zu行目で EOF (%s)\n", i, file_path.c_str());
      std::abort();
    }
    if(len == 0) {
      std::fprintf(stderr, "load_bin_winlose: %zu行目の履歴長が0 (%s)\n", i, file_path.c_str());
      std::abort();
    }
    std::string history(len, '\0');
    ifs.read(&history[0], len);
    if(!ifs) {
      std::fprintf(stderr, "load_bin_winlose: %zu行目の履歴を読み切る前に EOF (%s)\n", i, file_path.c_str());
      std::abort();
    }

    uint8_t flags = 0;
    ifs.read(reinterpret_cast<char*>(&flags), sizeof(flags));
    if(!ifs) {
      std::fprintf(stderr, "load_bin_winlose: %zu行目でフラグを読めなかった (%s)\n", i, file_path.c_str());
      std::abort();
    }
    if(flags & 0xF8) {
      std::fprintf(stderr, "load_bin_winlose: %zu行目の予約ビットが0でない (%s)\n", i, file_path.c_str());
      std::abort();
    }

    winlose_record row;
    row.history = std::move(history);
    row.is_win = flags & 0x1;
    row.slot = (flags & 0x2) ? 1 : 0;
    row.is_choice_node = flags & 0x4;
    rows.push_back(std::move(row));
  }
}
#endif
