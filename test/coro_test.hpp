#pragma once
#include <gtest/gtest.h>

#define ASSERT_CORO_RUNNING(coro) ASSERT_TRUE(!!(coro))
#define ASSERT_CORO_DEAD(coro) ASSERT_TRUE(!(coro))
