#include "command/Command.h"
#include "HookAllCommand.h"
#include "HookAll.h"
#include <iostream>
#include <memory>

void ll::HookAllCommand::init() {
    auto operateCommand = command::Command::instance()->addCommand("生成HookAll头文件", []() {
        std::string path;
        std::cout << "请输入ll源代码目录路径:";
        std::cin >> path;
        if (std::cin) {
            ll::HookAll::generate(removeQuotes(path));
            return true;
        } else {
            std::cin.clear();
            return false;
        }
    });
}

std::string& ll::HookAllCommand::removeQuotes(std::string& str) {
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
