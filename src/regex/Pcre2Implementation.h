#pragma once
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace regex {
    class Pcre2Implementation {
    public:
        class StringWithPosition {
        public:
            StringWithPosition(size_t start, size_t end, std::string_view text) : start(start), end(end), text(text) {
            }

            size_t start;
            size_t end;
            std::string text;
        };
        class MatchInfo {
        public:
            std::vector<StringWithPosition> group;
            std::unordered_map<std::string, size_t> namedGroup;
        };
        Pcre2Implementation(const std::string& pattern);
        ~Pcre2Implementation();
        std::expected<bool, std::string> exist(const std::string& text, size_t start = 0);
        std::expected<std::vector<MatchInfo>, std::string> find(const std::string& text, int times = 0, size_t start = 0);
        std::expected<std::string, std::string> replace(const std::string& originalText, const std::string& replacementText, int times = 0, size_t start = 0);
        std::expected<std::string, std::string> replace(const std::string& originalText, const std::string& replacementText, std::vector<MatchInfo>& matchInfoList, int times = 0, size_t start = 0);
    private:
        std::expected<bool, std::string> init();
    private:
        std::string pattern;
        class Pcre2Private;              // 前向声明私有实现类
        std::unique_ptr<Pcre2Private> d; // pimpl指针
    };
} // namespace regex
