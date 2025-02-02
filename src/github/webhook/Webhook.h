#pragma once
#include <string>
namespace github {
    namespace webhook {
        class Webhook {
        private:
            Webhook() = default;
            ~Webhook() = default;
            // 禁止外部拷贝构造
            Webhook(const Webhook& single) = delete;
            // 禁止外部赋值操作
            const Webhook& operator=(const Webhook& single) = delete;
        public:
            static std::string transform(std::string fileName);
        };
    } // namespace webhook
} // namespace github
