#pragma once
#include <string>
    namespace reverse {
        class ReverseCommand {
        private:
            ReverseCommand() = default;
            ~ReverseCommand() = default;
            // 禁止外部拷贝构造
            ReverseCommand(const ReverseCommand& single) = delete;
            // 禁止外部赋值操作
            const ReverseCommand &operator=(const ReverseCommand& single) = delete;

        public:
            static void init();
        };
    } // namespace reverse
