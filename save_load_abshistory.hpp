#ifndef SAVE_LOAD_ABSHISTORY_HPP
#define SAVE_LOAD_ABSHISTORY_HPP
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

void save_bin_abs(const std::string& filename,
                    const std::map<std::string, bool>& abs_history,
                    const std::set<std::string>& only_history)
{
    fs::path dir_path = "abs";

    // 2. ディレクトリが存在しない場合は自動で作成する
    if (!fs::exists(dir_path)) {
        fs::create_directories(dir_path);
    }

    // 3. ディレクトリパスとファイル名を結合 (例: "abs/history.bin")
    fs::path file_path = dir_path / filename;

    std::ofstream ofs(file_path, std::ios::binary);

    // 1. mapのサイズを書き込む
    size_t map_size = abs_history.size();
    ofs.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));

    // 2. mapの中身を書き込む
    for (const auto& [key, value] : abs_history) {
        // Key (string) の保存: [文字数] + [文字列データ]
        size_t key_size = key.size();
        ofs.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
        ofs.write(key.data(), key_size);

        // Value (bool) の保存
        ofs.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    // 3. setのサイズを書き込む
    size_t set_size = only_history.size();
    ofs.write(reinterpret_cast<const char*>(&set_size), sizeof(set_size));

    // 4. setの中身を書き込む
    for (const auto& item : only_history) {
        // 要素 (string) の保存: [文字数] + [文字列データ]
        size_t item_size = item.size();
        ofs.write(reinterpret_cast<const char*>(&item_size), sizeof(item_size));
        ofs.write(item.data(), item_size);
    }
}

void load_bin_abs(const std::string& filename,
                      std::map<std::string, bool>& abs_history,
                      std::set<std::string>& only_history)
{
    fs::path file_path = fs::path("abs") / filename;

    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs) {
        std::cerr << "ファイルを開けませんでした。" << std::endl;
        return;
    }

    abs_history.clear();
    only_history.clear();

    // 1. mapのサイズを読み込む
    size_t map_size = 0;
    ifs.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));

    // 2. mapの中身を読み込む
    for (size_t i = 0; i < map_size; ++i) {
        // Key の復元
        size_t key_size = 0;
        ifs.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
        std::string key(key_size, '\0');
        ifs.read(&key[0], key_size);

        // Value の復元
        bool value = false;
        ifs.read(reinterpret_cast<char*>(&value), sizeof(value));

        abs_history[key] = value;
    }

    // 3. setのサイズを読み込む
    size_t set_size = 0;
    ifs.read(reinterpret_cast<char*>(&set_size), sizeof(set_size));

    // 4. setの中身を読み込む
    for (size_t i = 0; i < set_size; ++i) {
        size_t item_size = 0;
        ifs.read(reinterpret_cast<char*>(&item_size), sizeof(item_size));
        std::string item(item_size, '\0');
        ifs.read(&item[0], item_size);

        only_history.insert(item);
    }
}
#endif