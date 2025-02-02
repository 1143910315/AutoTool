#pragma once
#include <string>
namespace ll {
    class HookAll {
    private:
        HookAll() = default;
        ~HookAll() = default;
        // 禁止外部拷贝构造
        HookAll(const HookAll& single) = delete;
        // 禁止外部赋值操作
        const HookAll& operator=(const HookAll& single) = delete;
    public:
        static std::string generate(std::string srcPath);
    };
} // namespace ll
