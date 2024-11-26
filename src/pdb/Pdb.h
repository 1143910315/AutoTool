#pragma once
#include <string>

    namespace pdb {
        class Pdb {
        private:
            Pdb() = default;
            ~Pdb() = default;
            // 禁止外部拷贝构造
            Pdb(const Pdb& single) = delete;
            // 禁止外部赋值操作
            const Pdb& operator=(const Pdb& single) = delete;
        public:
            static void getInfo(std::string fileName);
        };
    } // namespace pdb

