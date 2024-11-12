#pragma once
#include <expected>
#include <memory>
#include <string>
namespace regex {
    class Pcre2Implementation {
    public:
        Pcre2Implementation(std::string pattern);
        ~Pcre2Implementation();
        std::expected<bool, std::string> find(std::string text, size_t start = 0);
        std::expected<std::string, std::string> replace(std::string originalText, std::string replacementText, int replaceTimes = 0, size_t start = 0);
    private:
        std::expected<bool, std::string> init();
    private:
        class Pcre2Private;              // 前向声明私有实现类
        std::unique_ptr<Pcre2Private> d; // pimpl指针
        std::string pattern;
    };
} // namespace regex
