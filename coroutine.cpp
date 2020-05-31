#include <exception>

#include "coroutine.hpp"
#include "detail/libcoro/coro.h"

class coroutine_t::control_block_t final : public control_t {
public:
    explicit control_block_t(func_t, void* = nullptr);
    ~control_block_t();

    void resume();
    void yield() final;
    void* context() final {
        return queue_;
    }
    bool done() const {
        return stack_.sptr == nullptr || done_;
    }

private:
    static void body(void* this_);

private:
    coro_context ctx_main_;

    coro_stack stack_{nullptr, 0};
    coro_context ctx_;

    func_t func_;
    bool done_ = false;
    std::exception_ptr except_;
    void* queue_ = nullptr;
};

coroutine_t::control_block_t::control_block_t(
    coroutine_t::func_t func, void* queue)
    : func_(std::move(func)), queue_(queue) {
    coro_create(&ctx_main_, nullptr, nullptr, nullptr, 0);

    coro_stack_alloc(&stack_, 0);
    coro_create(&ctx_, body, this, stack_.sptr, stack_.ssze);
}

coroutine_t::control_block_t::~control_block_t() {
    coro_stack_free(&stack_);
}

void coroutine_t::control_block_t::resume() {
    auto prev = coroutine_t::current();
    coroutine_t::current(this);

    coro_transfer(&ctx_main_, &ctx_);

    coroutine_t::current(prev);

    if(except_)
        std::rethrow_exception(except_);
}

void coroutine_t::control_block_t::yield() {
    coro_transfer(&ctx_, &ctx_main_);
}

void coroutine_t::control_block_t::body(void* this_) {
    auto* this__ = static_cast<control_block_t*>(this_);

    try {
        this__->func_();
    } catch(...) {
        this__->except_ = std::current_exception();
    }

    this__->done_ = true;

    coro_transfer(&this__->ctx_, &this__->ctx_main_);
}

coroutine_t::coroutine_t(func_t func, void* queue)
    : cb_(new control_block_t(std::move(func), queue)) {
}

coroutine_t::coroutine_t(control_t* self)
    : cb_(static_cast<control_block_t*>(self)) {
}

coroutine_t::coroutine_t(coroutine_t&&) = default;
coroutine_t::~coroutine_t() = default;
coroutine_t& coroutine_t::operator=(coroutine_t&&) = default;

void coroutine_t::operator()() {
    cb_->resume();
}

void coroutine_t::resume(control_t* coro) {
    auto* cb = static_cast<control_block_t*>(coro);
    cb->resume();
}

coroutine_t::operator bool() const noexcept {
    return cb_ && !cb_->done();
}

coroutine_t::control_t* coroutine_t::release() {
    return cb_.release();
}

coroutine_t::control_t* coroutine_t::get() {
    return cb_.get();
}

thread_local coroutine_t::control_t* coroutine_t::current_ = nullptr;
