#pragma once
namespace utils {
    template<typename DeferFunction>
    class Defer {
    public:
        Defer(DeferFunction func) : func(func) {
        }

        ~Defer() {
            func();
        }

    private:
        DeferFunction func;
    };
}
