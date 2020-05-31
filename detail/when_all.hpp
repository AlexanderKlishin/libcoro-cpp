#include "coro_await.hpp"
#include "coroutine_cache.hpp"

namespace detail {

template<typename... AWAITABLES>
class awaitable_list_t {
public:
    using tasks_t = std::tuple<AWAITABLES...>;
    using value_type = std::tuple<typename AWAITABLES::value_type...>;

public:
    awaitable_list_t(tasks_t tasks)
        : coro_cache_(coroutine_cache_t::thread_cache())
        , tasks_(std::move(tasks))
        , wait_count_(std::tuple_size<tasks_t>::value) {
    }
    bool await_ready() const noexcept {
        return wait_count_ == 0;
    }
    void await_suspend() noexcept {
        coro_ = coroutine_t::current();
        await_all(std::make_index_sequence<std::tuple_size<tasks_t>::value>{});
    }
    value_type await_resume() noexcept {
        return std::move(res_);
    }

private:
    template<size_t I>
    void await_single() {
        coro_cache_.execute([this]() {
            std::get<I>(res_) = coro_await(std::move(std::get<I>(tasks_)));
            wait_count_--;
            if(wait_count_ == 0)
                coroutine_t::resume_via_queue(coro_);
        });
    }

    template<size_t... I>
    void await_all(std::index_sequence<I...>) {
        (await_single<I>(), ...);
    }

private:
    coroutine_cache_t& coro_cache_;
    tasks_t tasks_;
    value_type res_;
    size_t wait_count_ = 0;
    coroutine_t::control_t* coro_ = nullptr;
};

} // namespace detail
