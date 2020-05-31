#include "mutex.hpp"

bool async_recursive_st_mutex_t::scoped_lock_op_t::await_ready() const
    noexcept {
    // acquire or reenter
    if(!mx_.owner_ || mx_.owner_ == coro_) {
        mx_.owner_ = coro_;
        mx_.depth_ += 1;
        return true;
    }

    return false;
}

void async_recursive_st_mutex_t::scoped_lock_op_t::await_suspend() noexcept {
    if(!mx_.head_) {
        mx_.head_ = mx_.tail_ = this;
        return;
    }
    mx_.tail_->next_ = this;
    mx_.tail_ = this;
}

async_recursive_st_mutex_t::scoped_lock_t
async_recursive_st_mutex_t::scoped_lock_op_t::await_resume() const noexcept {
    return scoped_lock_t(mx_);
}

void async_recursive_st_mutex_t::unlock() {
    depth_ -= 1;
    if(depth_ > 0)
        return;

    owner_ = nullptr;

    // no waiting coros
    if(!head_)
        return;

    auto next = head_;
    head_ = head_->next_; // pop head
    next->next_ = nullptr;
    owner_ = next->coro_;
    depth_ += 1;
    coroutine_t::resume(owner_);
}
