#include <coro_await.hpp>
#include <gtest/gtest.h>

struct Request {
    coroutine_t::control_t* coro;
};

struct AwaitRequest {
    using value_type = int;

    AwaitRequest(Request* req, int res) : req(req), res(res) {
    }
    Request* req = nullptr;
    value_type res = -1;

    bool await_ready() {
        return false;
    }
    void await_suspend() {
        // make async request
        req->coro = coroutine_t::current();
    }
    value_type await_resume() {
        return res;
    }
};

TEST(coro_test_await, await_request) {
    int res = 0;
    Request request;
    Request* req = &request;
    coroutine_t coro([&res, req]() {
        res = coro_await(AwaitRequest(req, 1));
        res = coro_await(AwaitRequest(req, 2));
        res = coro_await(AwaitRequest(req, 3));
    });

    coro();
    ASSERT_EQ(0, res);
    ASSERT_TRUE(!!coro);

    coro();
    ASSERT_EQ(1, res);
    ASSERT_TRUE(!!coro);

    coro();
    ASSERT_EQ(2, res);
    ASSERT_TRUE(!!coro);

    coro();
    ASSERT_EQ(3, res);
    ASSERT_FALSE(coro);
}
