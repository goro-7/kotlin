/*
 * Copyright 2010-2022 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Clock.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ScopedThread.hpp"

using namespace kotlin;

TEST(ClockInternalTest, WaitUntilViaFor_Int_ImmediateOK) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    constexpr int okValue = 13;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(okValue));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, okValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Int_EventualOK) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    constexpr int okValue = 13;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(okValue));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, okValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Int_LastChanceOK) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    constexpr int okValue = 13;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step));
        EXPECT_CALL(waitForF, Call(rest)).WillOnce(testing::Return(okValue));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, okValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Int_Timeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step));
        EXPECT_CALL(waitForF, Call(rest)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step + rest));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, timeoutValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Int_ImmediateTimeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step + step));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, timeoutValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Int_ClockJumpTimeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        // Instead of incrementing by `step`, the clock jumped straight to `until`.
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(until));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, timeoutValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Int_NonconformantWaitTimeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    // waitFor non-conformingly waits less than specified.
    constexpr auto actualStep = std::chrono::seconds(7);
    constexpr auto rest = std::chrono::seconds(3);
    constexpr auto until = TimePoint() + step + step + rest;
    constexpr int timeoutValue = 42;
    testing::StrictMock<testing::MockFunction<int(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep));
        EXPECT_CALL(waitForF, Call(step)).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep + actualStep));
        // Waited 2 * 7 out of 2 * 10 + 3 seconds. 9 seconds left. Will wait only 6 seconds.
        EXPECT_CALL(waitForF, Call(std::chrono::seconds(9))).WillOnce(testing::Return(timeoutValue));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep + actualStep + std::chrono::seconds(6)));
        EXPECT_CALL(waitForF, Call(std::chrono::seconds(3))).WillOnce(testing::Return(timeoutValue));
        // Finally waited enough.
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep + actualStep + std::chrono::seconds(9)));
    }
    auto result = internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, timeoutValue, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
    EXPECT_THAT(result, timeoutValue);
}

TEST(ClockInternalTest, WaitUntilViaFor_Void_Timeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    testing::StrictMock<testing::MockFunction<void(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step));
        EXPECT_CALL(waitForF, Call(step));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step));
        EXPECT_CALL(waitForF, Call(rest));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step + rest));
    }
    internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
}

TEST(ClockInternalTest, WaitUntilViaFor_Void_ImmediateTimeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    testing::StrictMock<testing::MockFunction<void(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + step + step + step));
    }
    internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
}

TEST(ClockInternalTest, WaitUntilViaFor_Void_ClockJumpTimeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    constexpr auto rest = std::chrono::seconds(1);
    constexpr auto until = TimePoint() + step + step + rest;
    testing::StrictMock<testing::MockFunction<void(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step));
        // Instead of incrementing by `step`, the clock jumped straight to `until`.
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(until));
    }
    internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
}


TEST(ClockInternalTest, WaitUntilViaFor_Void_NonconformantWaitTimeout) {
    using TimePoint = std::chrono::time_point<test_support::manual_clock>;
    testing::StrictMock<testing::MockFunction<TimePoint()>> nowF;
    constexpr auto step = std::chrono::seconds(10);
    // waitFor non-conformingly waits less than specified.
    constexpr auto actualStep = std::chrono::seconds(7);
    constexpr auto rest = std::chrono::seconds(3);
    constexpr auto until = TimePoint() + step + step + rest;
    testing::StrictMock<testing::MockFunction<void(std::chrono::seconds)>> waitForF;

    {
        testing::InSequence s;
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint()));
        EXPECT_CALL(waitForF, Call(step));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep));
        EXPECT_CALL(waitForF, Call(step));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep + actualStep));
        // Waited 2 * 7 out of 2 * 10 + 3 seconds. 9 seconds left. Will wait only 6 seconds.
        EXPECT_CALL(waitForF, Call(std::chrono::seconds(9)));
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep + actualStep + std::chrono::seconds(6)));
        EXPECT_CALL(waitForF, Call(std::chrono::seconds(3)));
        // Finally waited enough.
        EXPECT_CALL(nowF, Call()).WillOnce(testing::Return(TimePoint() + actualStep + actualStep + std::chrono::seconds(9)));
    }
    internal::waitUntilViaFor(nowF.AsStdFunction(), step, until, [&](auto interval) {
        return waitForF.Call(std::chrono::duration_cast<std::chrono::seconds>(interval));
    });
}

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

} // namespace

template <typename T>
class ClockTest : public testing::Test {};

using ClockTestTypes = testing::Types<kotlin::steady_clock, kotlin::test_support::manual_clock>;
TYPED_TEST_SUITE(ClockTest, ClockTestTypes, TypesNames);

TYPED_TEST(ClockTest, SleepFor_Types) {
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<std::chrono::nanoseconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<std::chrono::microseconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<std::chrono::milliseconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<std::chrono::seconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<std::chrono::minutes>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<std::chrono::hours>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<kotlin::nanoseconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<kotlin::microseconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<kotlin::milliseconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<kotlin::seconds>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<kotlin::minutes>()))>);
    static_assert(std::is_same_v<void, decltype(TypeParam::sleep_for(std::declval<kotlin::hours>()))>);
}

TYPED_TEST(ClockTest, SleepFor) {
    constexpr auto interval = milliseconds(1);
    auto before = TypeParam::now();
    TypeParam::sleep_for(interval);
    auto after = TypeParam::now();
    EXPECT_THAT(after - before, testing::Ge(interval));
}

TYPED_TEST(ClockTest, SleepUntil_Types) {
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, std::chrono::nanoseconds>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, std::chrono::microseconds>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, std::chrono::milliseconds>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, std::chrono::seconds>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, std::chrono::minutes>>()))>);
    static_assert(
            std::is_same_v<void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, std::chrono::hours>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, kotlin::nanoseconds>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, kotlin::microseconds>>()))>);
    static_assert(std::is_same_v<
                  void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, kotlin::milliseconds>>()))>);
    static_assert(
            std::is_same_v<void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, kotlin::seconds>>()))>);
    static_assert(
            std::is_same_v<void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, kotlin::minutes>>()))>);
    static_assert(
            std::is_same_v<void, decltype(TypeParam::sleep_until(std::declval<std::chrono::time_point<TypeParam, kotlin::hours>>()))>);
}

TYPED_TEST(ClockTest, SleepUntil) {
    auto until = TypeParam::now() + milliseconds(1);
    TypeParam::sleep_until(until);
    auto after = TypeParam::now();
    EXPECT_THAT(after, testing::Ge(until));
}

TYPED_TEST(ClockTest, CVWaitFor_Types) {
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::nanoseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::microseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::milliseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::seconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::minutes>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::hours>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<kotlin::nanoseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<kotlin::microseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<kotlin::milliseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<kotlin::seconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<kotlin::minutes>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<kotlin::hours>(), std::declval<std::function<bool()>>()))>);
}

TYPED_TEST(ClockTest, CVWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::condition_variable cv;
    std::mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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

TYPED_TEST(ClockTest, CVWaitUntil_Types) {
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::nanoseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::microseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::milliseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::seconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::minutes>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::hours>>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::nanoseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::microseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::milliseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::seconds>>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::minutes>>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable&>(), std::declval<std::unique_lock<std::mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::hours>>(), std::declval<std::function<bool()>>()))>);
}

TYPED_TEST(ClockTest, CVWaitUntil_OK) {
    std::condition_variable cv;
    std::mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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

TYPED_TEST(ClockTest, CVAnyWaitFor_Types) {
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::nanoseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::microseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::milliseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::seconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::minutes>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::hours>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<kotlin::nanoseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<kotlin::microseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<kotlin::milliseconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<kotlin::seconds>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<kotlin::minutes>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_for(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<kotlin::hours>(), std::declval<std::function<bool()>>()))>);
}

TYPED_TEST(ClockTest, CVAnyWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::condition_variable_any cv;
    std::shared_mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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

TYPED_TEST(ClockTest, CVAnyWaitUntil_Types) {
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::nanoseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::microseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::milliseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::seconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::minutes>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::hours>>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::nanoseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::microseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::milliseconds>>(),
                          std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::seconds>>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::minutes>>(), std::declval<std::function<bool()>>()))>);
    static_assert(std::is_same_v<
                  bool,
                  decltype(TypeParam::wait_until(
                          std::declval<std::condition_variable_any&>(), std::declval<std::unique_lock<std::shared_mutex>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::hours>>(), std::declval<std::function<bool()>>()))>);
}

TYPED_TEST(ClockTest, CVAnyWaitUntil_OK) {
    std::condition_variable_any cv;
    std::shared_mutex m;
    bool ok = false;
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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

TYPED_TEST(ClockTest, FutureWaitFor_Types) {
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<std::chrono::nanoseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<std::chrono::microseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<std::chrono::milliseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<std::chrono::seconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<std::chrono::minutes>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<std::chrono::hours>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<kotlin::nanoseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<kotlin::microseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<kotlin::milliseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<kotlin::seconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<kotlin::minutes>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::future<int>&>(), std::declval<kotlin::hours>()))>);
}

TYPED_TEST(ClockTest, FutureWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
        // Wait to see if `TypeParam::wait_for` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_for(future, microseconds::max());
    EXPECT_THAT(result, std::future_status::ready);
}

TYPED_TEST(ClockTest, FutureWaitUntil_Types) {
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::nanoseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::microseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::milliseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::seconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::minutes>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::hours>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::nanoseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::microseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::milliseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(), std::declval<std::chrono::time_point<TypeParam, kotlin::seconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(), std::declval<std::chrono::time_point<TypeParam, kotlin::minutes>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::future<int>&>(), std::declval<std::chrono::time_point<TypeParam, kotlin::hours>>()))>);
}

TYPED_TEST(ClockTest, FutureWaitUntil_OK) {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
        // Wait to see if `TypeParam::wait_until` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_until(future, TypeParam::time_point::max());
    EXPECT_THAT(result, std::future_status::ready);
}

TYPED_TEST(ClockTest, SharedFutureWaitFor_Types) {
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<std::chrono::nanoseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(
                          TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<std::chrono::microseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(
                          TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<std::chrono::milliseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<std::chrono::seconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<std::chrono::minutes>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<std::chrono::hours>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<kotlin::nanoseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<kotlin::microseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<kotlin::milliseconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<kotlin::seconds>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<kotlin::minutes>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_for(std::declval<const std::shared_future<int>&>(), std::declval<kotlin::hours>()))>);
}

TYPED_TEST(ClockTest, SharedFutureWaitFor_OK) {
    constexpr auto interval = hours(10);
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
        // Wait to see if `TypeParam::wait_for` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_for(future, microseconds::max());
    EXPECT_THAT(result, std::future_status::ready);
}

TYPED_TEST(ClockTest, SharedFutureWaitUntil_Types) {
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::nanoseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::microseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::milliseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::seconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::minutes>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, std::chrono::hours>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::nanoseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::microseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::milliseconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::seconds>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::minutes>>()))>);
    static_assert(std::is_same_v<
                  std::future_status,
                  decltype(TypeParam::wait_until(
                          std::declval<const std::shared_future<int>&>(),
                          std::declval<std::chrono::time_point<TypeParam, kotlin::hours>>()))>);
}

TYPED_TEST(ClockTest, SharedFutureWaitUntil_OK) {
    std::promise<int> promise;
    std::shared_future<int> future = promise.get_future();
    std::atomic<bool> go = false;
    ScopedThread thread([&] {
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
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
        while (!go.load()) {
        }
        // Wait to see if `TypeParam::wait_until` wakes up from timeout.
        TypeParam::sleep_for(milliseconds(1));
        promise.set_value(42);
    });
    go = true;
    auto result = TypeParam::wait_until(future, TypeParam::time_point::max());
    EXPECT_THAT(result, std::future_status::ready);
}
