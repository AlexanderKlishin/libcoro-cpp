#include <tuple>

#include "detail/when_all.hpp"

template<typename... AWAITABLES>
inline auto when_all(AWAITABLES... awaitables) {
    return detail::awaitable_list_t(std::make_tuple(std::move(awaitables)...));
