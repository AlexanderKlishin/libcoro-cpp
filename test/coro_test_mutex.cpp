#include <mutex.hpp>

#include "coro_test.hpp"

namespace {

struct coro_mutex_recursive_st : public ::testing::Test {
    std::string res;
    async_recursive_st_mutex_t mx;
};

} // namespace

#define EXPECT_MX_ST(mx_depth) EXPECT_EQ(mx_depth, mx.depth());

TEST_F(coro_mutex_recursive_st, basic) {
    coroutine_t coro([this]() {
        auto guard = coro_await(mx.scoped_lock());
        coro_yield();
    });

    EXPECT_MX_ST(0);
    coro();
    EXPECT_MX_ST(1);
    coro();
    EXPECT_MX_ST(0);
    ASSERT_CORO_DEAD(coro);
}

TEST_F(coro_mutex_recursive_st, reenter) {
    coroutine_t coro([this]() {
        auto guard = coro_await(mx.scoped_lock());
        coro_yield();
        {
            auto guard = coro_await(mx.scoped_lock());
            coro_yield();
            {
                auto guard = coro_await(mx.scoped_lock());
                coro_yield();
            }
        }
    });

    EXPECT_MX_ST(0);
    coro();
    EXPECT_MX_ST(1);
    coro();
    EXPECT_MX_ST(2);
    coro();
    EXPECT_MX_ST(3);
    coro();
    EXPECT_MX_ST(0);
    ASSERT_CORO_DEAD(coro);
}

TEST_F(coro_mutex_recursive_st, two_coros_lock) {
    coroutine_t coro1([this]() {
        auto guard = coro_await(mx.scoped_lock());
        res += "1";
        coro_yield();
        res += "2";
    });

    coroutine_t coro2([this]() {
        auto guard = coro_await(mx.scoped_lock());
        res += "3";
        coro_yield();
        res += "4";
    });

    EXPECT_MX_ST(0);
    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro2();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("123", res.data());
    EXPECT_FALSE(coro1);

    coro2();
    EXPECT_MX_ST(0);
    ASSERT_STREQ("1234", res.data());
    EXPECT_FALSE(coro2);
}

TEST_F(coro_mutex_recursive_st, two_coros_lock_unlock) {
    coroutine_t coro1([this]() {
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "1";
            coro_yield();
        }
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "3";
            coro_yield();
        }
    });

    coroutine_t coro2([this]() {
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "2";
            coro_yield();
        }
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "4";
            coro_yield();
        }
    });

    EXPECT_MX_ST(0);

    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro2();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("12", res.data());

    coro2();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("123", res.data());

    coro1();
    ASSERT_FALSE(coro1);
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1234", res.data());

    coro2();
    ASSERT_FALSE(coro2);
    EXPECT_MX_ST(0);
    ASSERT_STREQ("1234", res.data());
}

TEST_F(coro_mutex_recursive_st, reentrant) {
    coroutine_t coro1([this]() {
        auto guard = coro_await(mx.scoped_lock());
        res += "1";
        coro_yield();
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "2";
            coro_yield();
            {
                auto guard = coro_await(mx.scoped_lock());
                res += "3";
                coro_yield();
            }
        }
    });

    coroutine_t coro2([this]() {
        auto guard = coro_await(mx.scoped_lock());
        res += "4";
        coro_yield();
    });

    EXPECT_MX_ST(0);
    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro2();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro1();
    EXPECT_MX_ST(2);
    ASSERT_STREQ("12", res.data());

    coro1();
    EXPECT_MX_ST(3);
    ASSERT_STREQ("123", res.data());

    coro1();
    ASSERT_FALSE(coro1);
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1234", res.data());

    coro2();
    ASSERT_FALSE(coro2);
    EXPECT_MX_ST(0);
}

TEST_F(coro_mutex_recursive_st, two_coros_lock_unlock_reenter) {
    coroutine_t coro1([this]() {
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "1";
            coro_yield();
        }
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "1";
            coro_yield();
        }
    });

    coroutine_t coro2([this]() {
        auto guard = coro_await(mx.scoped_lock());
        res += "a";
        coro_yield();
        {
            auto guard = coro_await(mx.scoped_lock());
            res += "b";
            coro_yield();
            {
                auto guard = coro_await(mx.scoped_lock());
                res += "c";
                coro_yield();
            }
        }
    });

    EXPECT_MX_ST(0);

    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro2();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1", res.data());

    coro1();
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1a", res.data());

    coro2();
    EXPECT_MX_ST(2);
    ASSERT_STREQ("1ab", res.data());

    coro2();
    EXPECT_MX_ST(3);
    ASSERT_STREQ("1abc", res.data());

    coro2();
    ASSERT_FALSE(coro2);
    EXPECT_MX_ST(1);
    ASSERT_STREQ("1abc1", res.data());

    coro1();
    ASSERT_FALSE(coro1);
    EXPECT_MX_ST(0);
}
