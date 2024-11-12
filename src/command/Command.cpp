#include "Command.h"
#include "CommandCallback.h"
#include <format>
#include <iostream>
#include <mutex>

std::shared_ptr<command::Command> command::Command::instance() {
    static std::once_flag flag;
    static std::shared_ptr<Command> single;
    std::call_once(flag, []() {
        single = std::make_shared<Command>();
    });
    return single;
}

std::shared_ptr<command::Command> command::Command::addCommand(std::string title) {
    commandList.push_back(std::make_tuple<std::string, std::shared_ptr<Command>>(std::string(title), std::make_shared<Command>()));
    return std::get<1>(commandList.back());
}

std::shared_ptr<command::Command> command::Command::addCommand(std::string title, std::function<bool()> callback) {
    commandList.push_back(std::make_tuple<std::string, std::shared_ptr<Command>>(std::string(title), std::make_shared<CommandCallback>(callback)));
    return std::get<1>(commandList.back());
}

bool command::Command::execute() {
    while (true) {
        if (commandList.empty()) {
            return true;
        }
        std::string outputString = "请输入要执行的命令序号：\n";
        for (size_t i = 0; i < commandList.size(); i++) {
            auto& [title, command] = commandList[i];
            outputString += std::format("{}.{}\n", i + 1, title);
        }
        outputString += "0.上一级\n-1.退出";
        std::cout << outputString << std::endl;
        int input;
        std::cin >> input;
        if (input > 0 && static_cast<size_t>(input) <= commandList.size()) {
            auto& [title, command] = commandList[static_cast<size_t>(input) - 1];
            if (!command->execute()) {
                return false;
            }
        } else if (input == 0) {
            return true;
        } else if (input == -1) {
            return false;
        }
    }
    return true;
}
