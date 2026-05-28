#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "bf_position.hpp"
#include "log_util.hpp"
// #include "endgame.hpp"

extern int char_to_action(char c);
extern int char_to_wizard(char c);
extern int char_to_twonum(char c);

using namespace std;

// State get_state_by_bfp(const bf_position& bfp) {
//     State s;
//     s.barrier = bfp.barrier1;
//     s.not7_flag = bfp.not7_flag_e;
//     s.lt5_flag_s = bfp.lt5_flag_s;
//     s.lt5_flag_e = bfp.lt5_flag_e;
//     s.open_flag_s = bfp.open_flag_s;
//     s.open_flag_e = bfp.open_flag_e;
//     s.sol_flag_s = bfp.sol_flag_s;
//     s.sol_flag_e = bfp.sol_flag_e;
//     if(bfp.hand0[0] >= bfp.hand0[1]){
//       s.hand[0] = bfp.hand0[0];
//       s.hand[1] = bfp.hand0[1];
//     }else{
//       s.hand[0] = bfp.hand0[1];
//       s.hand[1] = bfp.hand0[0];
//     }
//     for(int i = 0; i < 8; i++){
//       s.trash[i] = bfp.trash[i];
//     }
//     return s;
// }


void check_actionfile_history(int open[3]) {
    string filename = "losegame.txt";
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
            // struct bf_position bfp(open, actions_to_string(actions, true));
            // if(is_lose(bfp).second){
            //     for(auto x : actions)
            //         cout << x << " ";
            //     cout << endl;
            //     bfp.print();
            //     cout << "is lose:" << is_lose(bfp) << endl << endl;
            // }
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
        bf_position bfp(open, history_line);

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
  std::vector<int> all_actions = {10, 34, 40, 40, 30, 40, 46, 30, 40, 4542, 31, 414, 44021, 30};
  long unsigned int i = 0;
  std::vector<int> actions;
  while(i < all_actions.size()){
    actions.push_back(all_actions[i]);
    for(auto x : actions)
        cout << x << " ";
    cout << endl;
    struct bf_position bfp(open, actions_to_string(actions, true));
    bfp.print();
    // cout << "is win:" << is_win(bfp) << endl << endl;
    auto lose_actions = is_lose(bfp);
    // cout << "is lose:" << lose_actions[0].second << endl << endl;
    i++;
  }
}

// void compare_history(int open[3]) {
// //   std::vector<int> all_actions = {27, 40, 35, 4573, 43, 30};
//     std::vector<int> actions = {27, 44000, 31, 416, 46, 31, 410, 40, 30, 40, 40, 34, 44000};
//     for(auto x : actions)
//         cout << x << " ";
//     cout << endl;
//     struct bf_position bfp(open, actions_to_string(actions, true), true);
//     bfp.print();
//     cout << "is win:" << is_win(bfp).second << endl;
//     State s = get_state_by_bfp(bfp);
//     cout << get_hash(s) << endl;
//     s.print();
//     State s2 = decode_hash(get_hash(s));
//     cout << get_hash(s2) << endl;
//     s2.print();
//     cout << encode_hand_idx(s.hand[0], s.hand[1]) << endl;
//     cout << "abs win:" << abs_win(s2) << endl;
//     // cout << "is lose:" << is_lose(bfp) << endl;
// }


void check_history(int open[3]) {
//   std::vector<int> all_actions = {25, 43, 32, 45, 412, 30, 420, 420, 33, 40, 40, 31, 43, 40, 37};
    std::vector<int> actions = { 21, 421, 32, 421, 411, 35, 417, 43, 30 };
    for(auto x : actions)
        cout << x << " ";
    cout << endl;
    struct bf_position bfp(open, actions_to_string(actions, true));
    bfp.print();
    auto res = is_win(bfp);
    cout << "is win:" << res.first << endl;
    cout << "win moves:" << res.second << endl;
    // cout << "is lose:" << is_lose(bfp) << endl;
}

int main() {
    // const std::string filename = "check557.txt";
    // int open[3] = {4,4,6};
    int open[3] = {3,4,4};
    // check_file_history(filename, open);
    // check_actionfile_history(open);
    check_allhistory(open);
    // check_history(open);
    // compare_history(open);
    return 0;
}