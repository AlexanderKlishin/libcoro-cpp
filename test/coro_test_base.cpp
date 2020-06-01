#include <coro_await.hpp>

#include "coro_test.hpp"

TEST(coro_test, single_coro_resume) {
    int res = 0;
    coroutine_t coro([&res]() { res += 1; });
    ASSERT_EQ(0, res);
    ASSERT_CORO_RUNNING(coro);
    coro();
    ASSERT_CORO_DEAD(coro);
    ASSERT_EQ(1, res);
}

TEST(coro_test, single_coro_resume_yield) {
    int res = 0;
    coroutine_t coro([&res]() {
        res += 1;
        coro_yield();
        res += 1;
    });
    ASSERT_EQ(0, res);

    ASSERT_CORO_RUNNING(coro);

    coro();
    ASSERT_EQ(1, res);

    coro();
    ASSERT_EQ(2, res);

    ASSERT_CORO_DEAD(coro);
}

TEST(coro_test, single_coro_resume_yield_cycle) {
    int size = 1000;
    int res = 0;
    coroutine_t coro([&res, &size]() {
        for(int i = 0; i < size; ++i) {
            res += 1;
            coro_yield();
        }
    });

    for(int i = 1; i <= size; ++i) {
        ASSERT_CORO_RUNNING(coro);
        coro();
        ASSERT_EQ(i, res);
    }

    ASSERT_CORO_RUNNING(coro);
    coro();
    ASSERT_CORO_DEAD(coro);
}

TEST(coro_test, two_coro_resume) {
    int res = 0;

    coroutine_t coro0([&res]() { res += 1; });
    coroutine_t coro1([&res]() { res += 1; });
    ASSERT_EQ(0, res);

    ASSERT_CORO_RUNNING(coro0);
    ASSERT_CORO_RUNNING(coro1);

    coro0();
    ASSERT_CORO_DEAD(coro0);
    ASSERT_EQ(1, res);

    coro1();
    ASSERT_CORO_DEAD(coro1);
    ASSERT_EQ(2, res);
}

TEST(coro_test, two_coro_resume_yield) {
    int res = 0;
    coroutine_t coro0([&res]() {
        res += 1;
        coro_yield();
        res += 1;
    });
    coroutine_t coro1([&res]() {
        res += 1;
        coro_yield();
        res += 1;
    });
    ASSERT_EQ(0, res);

    coro0();
    ASSERT_EQ(1, res);
    coro1();
    ASSERT_EQ(2, res);
    coro0();
    ASSERT_EQ(3, res);
    coro1();
    ASSERT_EQ(4, res);

    ASSERT_CORO_DEAD(coro0);
    ASSERT_CORO_DEAD(coro1);
}

TEST(coro_test, move_ctor) {
    coroutine_t coro0([]() { coro_yield(); });

    ASSERT_CORO_RUNNING(coro0);
    coroutine_t coro1(std::move(coro0));
    ASSERT_CORO_DEAD(coro0);

    ASSERT_CORO_RUNNING(coro1);
    coro1();
    ASSERT_CORO_RUNNING(coro1);
    coro1();
    ASSERT_CORO_DEAD(coro1);
}

TEST(coro_test, exceptions) {
    coroutine_t coro([]() {
        int e = 0;
        throw e;
    });

    try {
        coro();
        FAIL() << "exception now thrown";
    }
    catch(int& e) {
        EXPECT_EQ(0, e);
    }
    catch(...) {
        FAIL() << "unknown exception";
    }
}

TEST(coro_test, move_operator) {
    coroutine_t coro0([]() { coro_yield(); });
    coroutine_t coro1([]() { coro_yield(); });

    coro1 = std::move(coro0);

    ASSERT_CORO_DEAD(coro0);

    ASSERT_CORO_RUNNING(coro1);
    coro1();
    ASSERT_CORO_RUNNING(coro1);
    coro1();
    ASSERT_CORO_DEAD(coro1);
}

TEST(coro_test, many_coro_resume_yield) {
    int size = 1000;
    int res = 0;

    std::vector<coroutine_t> coros;

    for(int i = 0; i < size; ++i) {
        coros.push_back(coroutine_t([&res]() {
            res += 1;
            coro_yield();
            res += 1;
        }));
    }

    for(auto& coro : coros)
        ASSERT_CORO_RUNNING(coro);

    int cur = 1;
    // run all coros until yield
    for(auto& coro : coros) {
        ASSERT_CORO_RUNNING(coro);
        coro();
        ASSERT_EQ(cur, res);
        cur += 1;
    }

    // finish all coros
    for(auto& coro : coros) {
        ASSERT_CORO_RUNNING(coro);
        coro();
        ASSERT_CORO_DEAD(coro);
        ASSERT_EQ(cur, res);
        cur += 1;
    }
}

TEST(coro_test, context) {
    void* res = nullptr;
    void* set = reinterpret_cast<void*>(1);
    coroutine_t coro(
        [&res]() { res = coroutine_t::current()->context(); }, set);

    coro();
    ASSERT_EQ(set, res);
}

TEST(coro_test, nested) {
    std::string res;
    coroutine_t coro_4([&res]() { res += "4"; });
    coroutine_t coro_3([&res, &coro_4]() {
        res += "3";
        coro_4();
        res += "3";
    });
    coroutine_t coro_2([&res, &coro_3]() {
        res += "2";
        coro_3();
        res += "2";
    });
    coroutine_t coro_1([&res, &coro_2]() {
        res += "1";
        coro_2();
        res += "1";
    });

    coro_1();
    ASSERT_STREQ("1234321", res.data());
}

#define CHECK_CUR(coro, depth)         \
    if(coroutine_t::current() != coro) \
        std::cout << "wrong current coro, depth = " << depth;
#define CHECK_FINISHED(coro, depth) \
    if(!!coro)                      \
        std::cout << "coro not finished, depth = " << depth;
TEST(coro_test, current_nested) {
    std::string err;
    coroutine_t coro_4([&err]() {
        auto coro = coroutine_t::current();
        coro_yield();
        CHECK_CUR(coro, 4);
    });
    coroutine_t coro_3([&err, &coro_4]() {
        auto coro = coroutine_t::current();
        coro_4();
        CHECK_CUR(coro, 3);
        coro_4();
        CHECK_CUR(coro, 3);
        CHECK_FINISHED(coro_4, 3);
    });
    coroutine_t coro_2([&err, &coro_3]() {
        auto coro = coroutine_t::current();
        coro_3();
        CHECK_CUR(coro, 2);
        CHECK_FINISHED(coro_3, 2);
    });
    coroutine_t coro_1([&err, &coro_2]() {
        auto coro = coroutine_t::current();
        coro_2();
        CHECK_CUR(coro, 1);
        CHECK_FINISHED(coro_2, 1);
    });

    CHECK_CUR(nullptr, 0);
    coro_1();
    CHECK_CUR(nullptr, 0);
    CHECK_FINISHED(coro_1, 0);
    ASSERT_STREQ("", err.data());
}

TEST(coro_test, current_cross_calls) {
    std::string err;

    coroutine_t coro_other([&err]() {
        auto coro = coroutine_t::current();
        coro_yield();
        CHECK_CUR(coro, 1);
    });
    coroutine_t coro([&err, &coro_other]() {
        auto coro = coroutine_t::current();
        coro_yield();
        CHECK_CUR(coro, 2);
        coro_other();
        CHECK_CUR(coro, 2);
    });

    CHECK_CUR(nullptr, 0);
    coro();
    ASSERT_CORO_RUNNING(coro);
    CHECK_CUR(nullptr, 0);

    coro_other();
    ASSERT_CORO_RUNNING(coro_other);
    CHECK_CUR(nullptr, 0);

    coro();
    EXPECT_EQ("", err);
    EXPECT_FALSE(coro);
    EXPECT_FALSE(coro_other);
}

TEST(coro_test, nested_cross_call) {
    coroutine_t coro_other([]() { coro_yield(); });
    coroutine_t coro([&coro_other]() {
        coro_yield();
        coro_other();
    });

    coro();
    ASSERT_CORO_RUNNING(coro);

    coro_other();
    ASSERT_CORO_RUNNING(coro_other);

    coro();
    EXPECT_FALSE(coro);
    EXPECT_FALSE(coro_other);
}

TEST(coro_test, when_all_algo_test) {
    std::string res;
    coroutine_t coro1([&res]() {
        res += "1";
        coro_yield();
        res += "1";
    });
    coroutine_t coro2([&res]() {
        res += "2";
        coro_yield();
        res += "2";
    });
    coroutine_t coro_main(
        [&coro1, &coro2, &res]() {
            res += "0";
            coro1();
            coro2();
            coro_yield();
            res += "0";
        });

    coro_main();
    ASSERT_CORO_RUNNING(coro_main);
    EXPECT_STREQ("012", res.data());

    coro1();
    ASSERT_CORO_DEAD(coro1);
    EXPECT_STREQ("0121", res.data());

    coro2();
    ASSERT_CORO_DEAD(coro2);
    EXPECT_STREQ("01212", res.data());

    coro_main();
    ASSERT_CORO_DEAD(coro_main);
    EXPECT_STREQ("012120", res.data());
}
