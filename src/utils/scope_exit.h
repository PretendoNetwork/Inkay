#pragma once
#include <functional>

// https://stackoverflow.com/a/61242721
template<typename F>
struct scope_exit
{
    F func;
    explicit scope_exit(F&& f): func(std::forward<F>(f)) {}
    ~scope_exit() { func(); }
};

template<typename F> scope_exit(F&& frv) -> scope_exit<F>;
