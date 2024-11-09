#pragma once
#include "Command.h"
namespace command {
    class CommandCallback : public Command {
    public:
        CommandCallback(std::function<bool()> callback);
        // 禁止外部拷贝构造
        CommandCallback(const CommandCallback& single) = delete;
        // 禁止外部赋值操作
        const CommandCallback& operator=(const CommandCallback& single) = delete;
        bool execute() override;
    private:

        std::function<bool()> callbackFunction;
    };
} // namespace Command
