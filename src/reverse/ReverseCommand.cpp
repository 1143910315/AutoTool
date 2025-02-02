#include "command/Command.h"
#include "ReverseCommand.h"
#include <iostream>
#include <memory>

void reverse::ReverseCommand::init() {
    auto operateReverseCommand = command::Command::instance()->addCommand("ida、x64dbg地址映射", []() {
        std::string path;
        std::cout << "请输入PDB文件路径:";
        std::cin >> path;
        if (std::cin) {
            return true;
        } else {
            std::cin.clear();
            return false;
        }
    });
}

