#pragma once
#include "coro_await.hpp"
#include "coroutine.hpp"

// single thread async recursive mutex
class async_recursive_st_mutex_t {
public:
    class scoped_lock_t {
    public:
        explicit scoped_lock_t(async_recursive_st_mutex_t& mx) : mx_(mx) {
        }
        ~scoped_lock_t() {
            mx_.unlock();
        }

    private:
        async_recursive_st_mutex_t& mx_;
    };

    class scoped_lock_op_t {
        friend async_recursive_st_mutex_t;

    public:
        using value_type = scoped_lock_t;

        explicit scoped_lock_op_t(async_recursive_st_mutex_t& mx)
            : mx_(mx), coro_(coroutine_t::current()) {
        }
        bool await_ready() const noexcept;
        void await_suspend() noexcept;
        [[nodiscard]] scoped_lock_t await_resume() const noexcept;

    private:
        async_recursive_st_mutex_t& mx_;
        coroutine_t::control_t* coro_ = nullptr;
        scoped_lock_op_t* next_ = nullptr;
    };

public:
    async_recursive_st_mutex_t() = default;
    async_recursive_st_mutex_t(const async_recursive_st_mutex_t&) = delete;
    async_recursive_st_mutex_t& operator=(const async_recursive_st_mutex_t&) =
        delete;
    async_recursive_st_mutex_t(async_recursive_st_mutex_t&&) = default;
    async_recursive_st_mutex_t& operator=(async_recursive_st_mutex_t&&) =
        default;

    scoped_lock_op_t scoped_lock() noexcept {
        return scoped_lock_op_t(*this);
    }
    void unlock();

    // monitor
    uint32_t depth() const noexcept {
        return depth_;
    }

private:
    coroutine_t::control_t* owner_ = nullptr;
    uint32_t depth_ = 0;

    scoped_lock_op_t* head_ = nullptr;
    scoped_lock_op_t* tail_ = nullptr;
};
