/*
 * Copyright 2010-2022 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Clock.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ScopedThread.hpp"

using namespace kotlin;

namespace {

class TypesNames {
public:
    template <typename T>
    static std::string GetName(int) {
        if constexpr (std::is_same_v<T, kotlin::steady_clock>) {
            return "steady_clock";
        } else if constexpr (std::is_same_v<T, kotlin::test_support::manual_clock>) {
            return "manual_clock";
        } else {
            return "unknown";
        }
    }
};

}

template <typename T>
class ClockTest : public testing::Test {};

using ClockTestTypes = testing::Types<kotlin::steady_clock, kotlin::test_support::manual_clock>;
TYPED_TEST_SUITE(ClockTest, ClockTestTypes, TypesNames);

TYPED_TEST(ClockTest, SleepFor) {
    constexpr auto interval = milliseconds(1);
    auto before = TypeParam::now();
    TypeParam::sleep_for(interval);
    auto after = TypeParam::now();
    EXPECT_THAT(after - before, testing::Ge(interval));
}

TYPED_TEST(ClockTest, SleepUntil) {
    auto until = TypeParam::now() + milliseconds(1);
    TypeParam::sleep_until(until);
    auto after = TypeParam::now();
    EXPECT_THAT(after, testing::Ge(until));
}

TYPED_TEST(ClockTest, CVWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::condition_variable cv;
    std::mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(cv, guard, interval, [&] { return ok; });
    auto after = TypeParam::now();
    EXPECT_TRUE(result);
    EXPECT_THAT(after - before, testing::Lt(interval));
}

TYPED_TEST(ClockTest, CVWaitFor_Timeout) {
    constexpr auto interval = microseconds(10);
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    std::unique_lock guard(m);
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(cv, guard, interval, [] { return false; });
    auto after = TypeParam::now();
    EXPECT_FALSE(result);
    EXPECT_THAT(after - before, testing::Ge(interval));
}

TYPED_TEST(ClockTest, CVWaitFor_InfiniteTimeout) {
    std::condition_variable cv;
    std::mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_for` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    go = true;
    auto result = TypeParam::wait_for(cv, guard, microseconds::max(), [&] { return ok; });
    EXPECT_TRUE(result);
}

TYPED_TEST(ClockTest, CVWaitUntil_OK) {
    std::condition_variable cv;
    std::mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    auto until = TypeParam::now() + hours(10);
    go = true;
    auto result = TypeParam::wait_until(cv, guard, until, [&] { return ok; });
    auto after = TypeParam::now();
    EXPECT_TRUE(result);
    EXPECT_THAT(after, testing::Lt(until));
}

TYPED_TEST(ClockTest, CVWaitUntil_Timeout) {
    constexpr auto interval = microseconds(10);
    std::condition_variable cv;
    std::mutex m;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    std::unique_lock guard(m);
    auto until = TypeParam::now() + interval;
    go = true;
    auto result = TypeParam::wait_until(cv, guard, until, [] { return false; });
    auto after = TypeParam::now();
    EXPECT_FALSE(result);
    EXPECT_THAT(after, testing::Ge(until));
}

TYPED_TEST(ClockTest, CVWaitUntil_InfiniteTimeout) {
    std::condition_variable cv;
    std::mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_until` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    go = true;
    auto result = TypeParam::wait_until(cv, guard, TypeParam::time_point::max(), [&] { return ok; });
    EXPECT_TRUE(result);
}

TYPED_TEST(ClockTest, CVAnyWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::condition_variable_any cv;
    std::shared_mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(cv, guard, interval, [&] { return ok; });
    auto after = TypeParam::now();
    EXPECT_TRUE(result);
    EXPECT_THAT(after - before, testing::Lt(interval));
}

TYPED_TEST(ClockTest, CVAnyWaitFor_Timeout) {
    constexpr auto interval = microseconds(10);
    std::condition_variable_any cv;
    std::shared_mutex m;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    std::unique_lock guard(m);
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(cv, guard, interval, [] { return false; });
    auto after = TypeParam::now();
    EXPECT_FALSE(result);
    EXPECT_THAT(after - before, testing::Ge(interval));
}

TYPED_TEST(ClockTest, CVAnyWaitFor_InfiniteTimeout) {
    std::condition_variable_any cv;
    std::shared_mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_for` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    go = true;
    auto result = TypeParam::wait_for(cv, guard, microseconds::max(), [&] { return ok; });
    EXPECT_TRUE(result);
}

TYPED_TEST(ClockTest, CVAnyWaitUntil_OK) {
    std::condition_variable_any cv;
    std::shared_mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    auto until = TypeParam::now() + hours(10);
    go = true;
    auto result = TypeParam::wait_until(cv, guard, until, [&] { return ok; });
    auto after = TypeParam::now();
    EXPECT_TRUE(result);
    EXPECT_THAT(after, testing::Lt(until));
}

TYPED_TEST(ClockTest, CVAnyWaitUntil_Timeout) {
    constexpr auto interval = microseconds(10);
    std::condition_variable_any cv;
    std::shared_mutex m;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    std::unique_lock guard(m);
    auto until = TypeParam::now() + interval;
    go = true;
    auto result = TypeParam::wait_until(cv, guard, until, [] { return false; });
    auto after = TypeParam::now();
    EXPECT_FALSE(result);
    EXPECT_THAT(after, testing::Ge(until));
}

TYPED_TEST(ClockTest, CVAnyWaitUntil_InfiniteTimeout) {
    std::condition_variable_any cv;
    std::shared_mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_until` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        {
            std::unique_lock guard(m);
            ok = true;
        }
        cv.notify_all();
    });
    std::unique_lock guard(m);
    go = true;
    auto result = TypeParam::wait_until(cv, guard, TypeParam::time_point::max(), [&] { return ok; });
    EXPECT_TRUE(result);
}

TYPED_TEST(ClockTest, FutureWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        promise.set_value(42);
    });
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(future, interval);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::ready);
    EXPECT_THAT(after - before, testing::Lt(interval));
}

TYPED_TEST(ClockTest, FutureWaitFor_Deferred) {
    constexpr auto interval = hours(10);
    std::future<int> future = std::async(std::launch::deferred, [] { return 42; });
    auto before = TypeParam::now();
    auto result = TypeParam::wait_for(future, interval);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::deferred);
    EXPECT_THAT(after - before, testing::Lt(interval));
}

TYPED_TEST(ClockTest, FutureWaitFor_Timeout) {
    constexpr auto interval = microseconds(10);
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(future, interval);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::timeout);
    EXPECT_THAT(after - before, testing::Ge(interval));
}

TYPED_TEST(ClockTest, FutureWaitFor_InfiniteTimeout) {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_for` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_for(future, microseconds::max());
    EXPECT_THAT(result, std::future_status::ready);
}

TYPED_TEST(ClockTest, FutureWaitUntil_OK) {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        promise.set_value(42);
    });
    auto until = TypeParam::now() + hours(10);
    go = true;
    auto result = TypeParam::wait_until(future, until);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::ready);
    EXPECT_THAT(after, testing::Lt(until));
}

TYPED_TEST(ClockTest, FutureWaitUntil_Deferred) {
    std::future<int> future = std::async(std::launch::deferred, [] { return 42; });
    auto until = TypeParam::now() + hours(10);
    auto result = TypeParam::wait_until(future, until);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::deferred);
    EXPECT_THAT(after, testing::Lt(until));
}

TYPED_TEST(ClockTest, FutureWaitUntil_Timeout) {
    constexpr auto interval = microseconds(10);
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    auto until = TypeParam::now() + interval;
    go = true;
    auto result = TypeParam::wait_until(future, until);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::timeout);
    EXPECT_THAT(after, testing::Ge(until));
}

TYPED_TEST(ClockTest, FutureWaitUntil_InfiniteTimeout) {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_until` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_until(future, TypeParam::time_point::max());
    EXPECT_THAT(result, std::future_status::ready);
}

TYPED_TEST(ClockTest, SharedFutureWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        promise.set_value(42);
    });
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(future, interval);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::ready);
    EXPECT_THAT(after - before, testing::Lt(interval));
}

TYPED_TEST(ClockTest, SharedFutureWaitFor_Deferred) {
    constexpr auto interval = hours(10);
    std::shared_future<int> future = std::async(std::launch::deferred, [] { return 42; });
    auto before = TypeParam::now();
    auto result = TypeParam::wait_for(future, interval);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::deferred);
    EXPECT_THAT(after - before, testing::Lt(interval));
}

TYPED_TEST(ClockTest, SharedFutureWaitFor_Timeout) {
    constexpr auto interval = microseconds(10);
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    auto before = TypeParam::now();
    go = true;
    auto result = TypeParam::wait_for(future, interval);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::timeout);
    EXPECT_THAT(after - before, testing::Ge(interval));
}

TYPED_TEST(ClockTest, SharedFutureWaitFor_InfiniteTimeout) {
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_for` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_for(future, microseconds::max());
    EXPECT_THAT(result, std::future_status::ready);
}

TYPED_TEST(ClockTest, SharedFutureWaitUntil_OK) {
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        promise.set_value(42);
    });
    auto until = TypeParam::now() + hours(10);
    go = true;
    auto result = TypeParam::wait_until(future, until);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::ready);
    EXPECT_THAT(after, testing::Lt(until));
}

TYPED_TEST(ClockTest, SharedFutureWaitUntil_Deferred) {
    std::shared_future<int> future = std::async(std::launch::deferred, [] { return 42; });
    auto until = TypeParam::now() + hours(10);
    auto result = TypeParam::wait_until(future, until);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::deferred);
    EXPECT_THAT(after, testing::Lt(until));
}

TYPED_TEST(ClockTest, SharedFutureWaitUntil_Timeout) {
    constexpr auto interval = microseconds(10);
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        TypeParam::sleep_for(interval);
    });
    auto until = TypeParam::now() + interval;
    go = true;
    auto result = TypeParam::wait_until(future, until);
    auto after = TypeParam::now();
    EXPECT_THAT(result, std::future_status::timeout);
    EXPECT_THAT(after, testing::Ge(until));
}

TYPED_TEST(ClockTest, SharedFutureWaitUntil_InfiniteTimeout) {
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {}
        // Wait to see if `TypeParam::wait_until` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_until(future, TypeParam::time_point::max());
    EXPECT_THAT(result, std::future_status::ready);
}
