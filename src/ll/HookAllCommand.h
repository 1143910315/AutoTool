#pragma once
#include <string>
    namespace ll {
        class HookAllCommand {
        private:
            HookAllCommand() = default;
            ~HookAllCommand() = default;
            // 禁止外部拷贝构造
            HookAllCommand(const HookAllCommand& single) = delete;
            // 禁止外部赋值操作
            const HookAllCommand &operator=(const HookAllCommand& single) = delete;

        public:
            static void init();

        private:
            static std::string &removeQuotes(std::string& str);
        };
    } // namespace ll
