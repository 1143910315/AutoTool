#pragma once
#include <string>
    namespace pdb {
        class PdbCommand {
        private:
            PdbCommand() = default;
            ~PdbCommand() = default;
            // 禁止外部拷贝构造
            PdbCommand(const PdbCommand& single) = delete;
            // 禁止外部赋值操作
            const PdbCommand &operator=(const PdbCommand& single) = delete;

        public:
            static void init();

        private:
            static std::string &removeQuotes(std::string& str);
        };
    } // namespace pdb
