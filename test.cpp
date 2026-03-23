#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "bf_position.hpp"
#include "log_util.hpp"

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);

using namespace std;

void check_actionfile_history(int open[3]) {
    string filename = "wingame2.txt";
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "エラー: ファイル \"" << filename << "\" を開けませんでした。" << std::endl;
        return;
    }

    std::string line;
    int line_count = 0;

    while (std::getline(ifs, line)) {
        line_count++;
        std::vector<int> actions;

        std::stringstream ss(line);
        int action_value;

        while (ss >> action_value) {
            actions.push_back(action_value);
        }

        if (!actions.empty()) {
            struct bf_position bfp(open, actions_to_string(actions, true), true);
            if(is_win(bfp)){
                bfp.print();
            // cout << "is win:" << bfp.is_win() << endl << endl;
            }
        }
    }
    // 8. ファイルストリームを閉じる
    ifs.close();
}

void check_file_history(const std::string& filename, int open[3]) {
    // 1. ファイル入力ストリームを開く
    std::ifstream input_file(filename);

    if (!input_file.is_open()) {
        std::cerr << "エラー: ファイル \"" << filename << "\" を開けませんでした。" << std::endl;
        return;
    }

    std::string history_line;
    int line_number = 0;

    // 2. ファイルを一行ずつ読み込む
    while (std::getline(input_file, history_line)) {
        line_number++;

        // 3. オブジェクトの生成
        bf_position bfp(open, history_line, true);

        // 4. 指定された条件をチェック
        std::cout << "(行: " << line_number << "):" << std::endl;
        std::cout << "------------------------" << std::endl;
    }

    // 5. ファイルを閉じる
    input_file.close();

    std::cout << "\nファイルチェックが完了しました。" << std::endl;
}

void check_allhistory(int open[3]) {
//   std::vector<int> all_actions = {27, 40, 35, 4573, 43, 30};
  std::vector<int> all_actions = {27, 40, 31, 410, 40, 30, 40, 40, 35, 4572, 43, 32, 42, 43, 30};
  long unsigned int i = 0;
  std::vector<int> actions;
  while(i < all_actions.size()){
    actions.push_back(all_actions[i]);
    for(auto x : actions)
        cout << x << " ";
    cout << endl;
    struct bf_position bfp(open, actions_to_string(actions, true), true);
    bfp.print();
    cout << "is win:" << is_win(bfp) << endl << endl;
    i++;
  }
}

void check_history(int open[3]) {
//   std::vector<int> all_actions = {27, 40, 35, 4573, 43, 30};
    std::vector<int> actions = {14, 30, 44100, 40, 31, 417, 43, 34};
    for(auto x : actions)
        cout << x << " ";
    cout << endl;
    struct bf_position bfp(open, actions_to_string(actions, true), true);
    bfp.print();
    cout << "is win:" << is_win(bfp) << endl;
}

int main() {
    const std::string filename = "check557.txt";
    int open[3] = {5,5,7};
    // check_file_history(filename, open);
    // check_history(open);
    check_allhistory(open);
    // check_actionfile_history(open);

    return 0;
}