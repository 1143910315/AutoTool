#include "command/Command.h"
#include "Pdb.h"
#include "PdbCommand.h"
#include <iostream>
#include <memory>

void pdb::PdbCommand::init() {
    auto operatePdbCommand = command::Command::instance()->addCommand("输出PDB文件信息", []() {
        std::string path;
        std::cout << "请输入PDB文件路径:";
        std::cin >> path;
        if (std::cin) {
            Pdb::getInfo(removeQuotes(path));
            return true;
        } else {
            std::cin.clear();
            return false;
        }
    });
}

std::string& pdb::PdbCommand::removeQuotes(std::string& str) {
    if (!str.empty()) {
        // 检查并去除开头的双引号
        if (str.front() == '"') {
            str.erase(0, 1);
        }

        // 检查并去除结尾的双引号
        if (!str.empty() && str.back() == '"') {
            str.pop_back();
        }
    }
    return str;
}
