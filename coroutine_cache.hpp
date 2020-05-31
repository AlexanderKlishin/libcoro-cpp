#pragma once
#include <list>
#include "coroutine.hpp"
#include "coro_await.hpp"

class coroutine_cache_t {
private:
    class elem_t {
    public:
        explicit elem_t(coroutine_cache_t& pool, void* context)
            : cache_(pool)
            , coro_(
                  [this]() {
                      while(1) {
                          func_();

                          // ready for another task
                          func_ = nullptr;
                          cache_.release(*this);
                          coro_yield();
                      }
                  },
                  context) {
        }

        void operator()() {
            coro_();
        }

        template<class F>
        void set(F func) {
            func_ = std::move(func);
        }

        void list_remove() {
            if(prev_)
                prev_->next_ = next_;
            if(next_)
                next_->prev_ = prev_;
        }
        void list_insert(elem_t* cur) {
            prev_ = cur->prev_;
            next_ = cur;
            if(cur->prev_)
                cur->prev_->next_ = this;
            cur->prev_ = this;
        }

    private:
        coroutine_cache_t& cache_;
        coroutine_t::func_t func_;
        coroutine_t coro_;

        elem_t* prev_ = nullptr;
        elem_t* next_ = nullptr;
    };

public:
    coroutine_cache_t(void* context = nullptr) : context_(context) {
    }
    coroutine_cache_t(const coroutine_cache_t&) = delete;
    coroutine_cache_t& operator=(coroutine_cache_t&) = delete;

    template<class F>
    void execute(F func) {
        elem_t* coro = nullptr;
        if(free_) {
            coro = free_;
            free_->list_remove();
            free_size_ -= 1;
        } else {
            coro = new elem_t(*this, context_);
        }

        if(running_)
            coro->list_insert(running_);
        else
            running_ = coro;
        running_size_ += 1;

        coro->set(std::move(func));
        (*coro)();
    }

    size_t size() const {
        return free_size_ + running_size_;
    }

    size_t sizeRunning() const {
        return running_size_;
    }

    void context(void* context) {
        context_ = context;
    }

    static coroutine_cache_t& thread_cache() {
        static thread_local coroutine_cache_t cache;
        return cache;
    }

private:
    void release(elem_t& coro) {
        coro.list_remove();
        running_size_ -= 1;

        if(free_)
            coro.list_insert(free_);
        else
            free_ = &coro;
        free_size_ += 1;
    }

private:
    elem_t* free_ = nullptr;
    size_t free_size_ = 0;
    elem_t* running_  = nullptr;
    size_t running_size_ = 0;
    void* context_ = nullptr;
};
