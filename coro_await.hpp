#pragma once
#include "coroutine.hpp"

inline void coro_yield() {
    auto coro = coroutine_t::current();
    coro->yield();
}

template<typename V>
inline typename V::value_type coro_await(V&& v) {
    if(!v.await_ready()) {
        v.await_suspend();
        coro_yield();
    }
    return v.await_resume();
}
