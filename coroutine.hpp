#pragma once

#include <functional>
#include <memory>

class coroutine_t {
public:
    class control_t {
    public:
        virtual void yield() = 0;
        virtual void* context() = 0;
    };

    using func_t = std::function<void()>;

public:
    explicit coroutine_t(func_t func, void* context = nullptr);
    explicit coroutine_t(control_t* self);
    coroutine_t(coroutine_t&&);
    coroutine_t() = delete;
    coroutine_t(const coroutine_t&) = delete;
    ~coroutine_t();

    coroutine_t& operator=(coroutine_t&) = delete;
    coroutine_t& operator=(coroutine_t&&);

    void operator()(); // resume
    static void resume(control_t*);

    explicit operator bool() const noexcept;

    control_t* release();
    control_t* get();

    static control_t* current() {
        return current_;
    }

private:
    static void current(control_t* coro) {
        current_ = coro;
    }

private:
    class control_block_t;
    std::unique_ptr<control_block_t> cb_;
    static thread_local control_t* current_;
};
