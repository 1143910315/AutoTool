#pragma once
#include <string>
namespace github {
    namespace webhook {
        class WebhookCommand {
        private:
            WebhookCommand() = default;
            ~WebhookCommand() = default;
            // 禁止外部拷贝构造
            WebhookCommand(const WebhookCommand& single) = delete;
            // 禁止外部赋值操作
            const WebhookCommand& operator=(const WebhookCommand& single) = delete;
        public:
            static void init();
        private:
            static std::string& removeQuotes(std::string& str);
        };
    } // namespace webhook
}     // namespace github
