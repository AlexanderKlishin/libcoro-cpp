#include <gtest/gtest.h>
#include <coroutine_cache.hpp>

TEST(coroutine_cache, simple) {
    coroutine_cache_t cache;

    int res = 0;
    cache.execute([&res]() { res = 1; });
    ASSERT_EQ(1, res);

    cache.execute([&res]() { res = 2; });
    ASSERT_EQ(2, res);

    ASSERT_EQ(1, cache.size());
    ASSERT_EQ(0, cache.sizeRunning());
}

TEST(coroutine_cache, reuse) {
    std::vector<coroutine_t::control_t*> coros;

    auto func = [&coros]() {
        auto coro = coroutine_t::current();
        if(std::find(coros.begin(), coros.end(), coro) == coros.end())
            coros.push_back(coro);

        // do some work 1
        coro_yield();
        // do some work 2
    };

    coroutine_cache_t cache;

    ASSERT_EQ(0, cache.size());
    ASSERT_EQ(0, cache.sizeRunning());
    ASSERT_EQ(0, coros.size());

    cache.execute(func);
    ASSERT_EQ(1, cache.size());
    ASSERT_EQ(1, cache.sizeRunning());
    ASSERT_EQ(1, coros.size());

    coroutine_t::resume(coros[0]);
    ASSERT_EQ(1, cache.size());
    ASSERT_EQ(0, cache.sizeRunning());

    // second iteration - the same coro
    cache.execute(func);
    ASSERT_EQ(1, cache.size());
    ASSERT_EQ(1, cache.sizeRunning());
    ASSERT_EQ(1, coros.size());

    coroutine_t::resume(coros[0]);
    ASSERT_EQ(1, cache.size());
    ASSERT_EQ(0, cache.sizeRunning());
}

TEST(coroutine_cache, many_tasks) {
    std::vector<coroutine_t::control_t*> coros;

    auto func = [&coros]() {
        auto coro = coroutine_t::current();
        if(std::find(coros.begin(), coros.end(), coro) == coros.end())
            coros.push_back(coro);

        // do some work 1
        coro_yield();
        // do some work 2
    };

    coroutine_cache_t cache;

    ASSERT_EQ(0, cache.size());
    ASSERT_EQ(0, cache.sizeRunning());
    ASSERT_EQ(0, coros.size());

    const int count = 1000;

    for(int i = 0; i < count; ++i) {
        cache.execute(func);
        ASSERT_EQ(i + 1, cache.size());
        ASSERT_EQ(i + 1, cache.sizeRunning());
        ASSERT_EQ(i + 1, coros.size());
    }

    for(int i = 0; i < count; ++i) {
        coroutine_t::resume(coros[i]);
        ASSERT_EQ(count, cache.size());
        ASSERT_EQ(count - i - 1, cache.sizeRunning());
    }
}

TEST(coroutine_cache, context) {
    void* res = nullptr;
    void* set = reinterpret_cast<void*>(1);
    coroutine_cache_t cache(set);
    cache.execute([&res]() { res = coroutine_t::current()->context(); });
    ASSERT_EQ(set, res);
}
