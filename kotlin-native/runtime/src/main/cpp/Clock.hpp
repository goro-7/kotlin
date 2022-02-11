/*
 * Copyright 2010-2022 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include "Saturating.hpp"

namespace kotlin {

namespace internal {

template <typename NowF, typename StepDuration, typename Clock, typename Rep, typename Period, typename Ret, typename WaitForF>
Ret waitUntilViaFor(
        NowF&& nowF,
        StepDuration step,
        std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>> until,
        Ret timeoutValue,
        WaitForF&& waitForF) {
    while (true) {
        auto current = std::invoke(std::forward<NowF>(nowF));
        if (current >= until) {
            return timeoutValue;
        }
        auto left = until - current;
        if (left > step) {
            if (auto value = std::invoke(std::forward<WaitForF>(waitForF), step); value != timeoutValue) {
                return value;
            }
            continue;
        }
        // Shield standard library from saturating types.
        return std::invoke(std::forward<WaitForF>(waitForF), std::chrono::duration<Rep, Period>(left));
    }
}

template <typename NowF, typename StepDuration, typename Clock, typename Rep, typename Period, typename WaitForF>
void waitUntilViaFor(
        NowF&& nowF,
        StepDuration step,
        std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>> until,
        WaitForF&& waitForF) {
    while (true) {
        auto current = std::invoke(std::forward<NowF>(nowF));
        if (current >= until) {
            return;
        }
        auto left = until - current;
        if (left > step) {
            std::invoke(std::forward<WaitForF>(waitForF), step);
            continue;
        }
        // Shield standard library from saturating types.
        return std::invoke(std::forward<WaitForF>(waitForF), std::chrono::duration<Rep, Period>(left));
    }
}

template <typename Clock>
class ClockWaitImpl {
public:
    template <typename Rep, typename Period, typename F>
    static bool wait_for(
            std::condition_variable& cv, std::unique_lock<std::mutex>& lock, std::chrono::duration<Rep, Period> interval, F&& f) {
        // Not using cv.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        return wait_until(cv, lock, Clock::now() + interval, std::forward<F>(f));
    }

    template <typename Rep, typename Period, typename F>
    static bool wait_until(
            std::condition_variable& cv,
            std::unique_lock<std::mutex>& lock,
            std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>> until,
            F&& f) {
        return wait_until(
                cv, lock, std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>>(until), std::forward<F>(f));
    }

    template <typename Rep, typename Period, typename F>
    static bool wait_until(
            std::condition_variable& cv,
            std::unique_lock<std::mutex>& lock,
            std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>> until,
            F&& f) {
        // Implement in terms of repeated cv.wait_for of non-"infinite" intervals.
        return internal::waitUntilViaFor(&Clock::now, Clock::wait_step, until, false, [&](auto interval) {
            return cv.wait_for(lock, interval, std::forward<F>(f));
        });
    }

    template <typename Lock, typename Rep, typename Period, typename F>
    static bool wait_for(std::condition_variable_any& cv, Lock& lock, std::chrono::duration<Rep, Period> interval, F&& f) {
        // Not using cv.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        return wait_until(cv, lock, Clock::now() + interval, std::forward<F>(f));
    }

    template <typename Lock, typename Rep, typename Period, typename F>
    static bool wait_until(
            std::condition_variable_any& cv, Lock& lock, std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>> until, F&& f) {
        return wait_until(
                cv, lock, std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>>(until), std::forward<F>(f));
    }

    template <typename Lock, typename Rep, typename Period, typename F>
    static bool wait_until(
            std::condition_variable_any& cv,
            Lock& lock,
            std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>> until,
            F&& f) {
        // Implement in terms of repeated cv.wait_for of non-"infinite" intervals.
        return internal::waitUntilViaFor(&Clock::now, Clock::wait_step, until, false, [&](auto interval) {
            return cv.wait_for(lock, interval, std::forward<F>(f));
        });
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_for(const std::future<T>& future, std::chrono::duration<Rep, Period> interval) {
        // Not using future.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        return wait_until(future, Clock::now() + interval);
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_until(
            const std::future<T>& future, std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>> until) {
        return wait_until(future, std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>>(until));
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_until(
            const std::future<T>& future, std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>> until) {
        // Implement in terms of repeated future.wait_for of non-"infinite" intervals.
        return internal::waitUntilViaFor(&Clock::now, Clock::wait_step, until, std::future_status::timeout, [&](auto interval) {
            return future.wait_for(interval);
        });
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_for(const std::shared_future<T>& future, std::chrono::duration<Rep, Period> interval) {
        // Not using future.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        return wait_until(future, Clock::now() + interval);
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_until(
            const std::shared_future<T>& future, std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>> until) {
        return wait_until(future, std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>>(until));
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_until(
            const std::shared_future<T>& future, std::chrono::time_point<Clock, std::chrono::duration<saturating<Rep>, Period>> until) {
        // Implement in terms of repeated future.wait_for of non-"infinite" intervals.
        return internal::waitUntilViaFor(&Clock::now, Clock::wait_step, until, std::future_status::timeout, [&](auto interval) {
            return future.wait_for(interval);
        });
    }
};

} // namespace internal

using nanoseconds = std::chrono::duration<saturating<std::chrono::nanoseconds::rep>, std::chrono::nanoseconds::period>;
using microseconds = std::chrono::duration<saturating<std::chrono::microseconds::rep>, std::chrono::microseconds::period>;
using milliseconds = std::chrono::duration<saturating<std::chrono::milliseconds::rep>, std::chrono::milliseconds::period>;
using seconds = std::chrono::duration<saturating<std::chrono::seconds::rep>, std::chrono::seconds::period>;
using minutes = std::chrono::duration<saturating<std::chrono::minutes::rep>, std::chrono::minutes::period>;
using hours = std::chrono::duration<saturating<std::chrono::hours::rep>, std::chrono::hours::period>;

class steady_clock : public internal::ClockWaitImpl<steady_clock> {
public:
    using rep = saturating<std::chrono::steady_clock::rep>;
    using period = std::chrono::steady_clock::period;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<steady_clock>;

    static constexpr bool is_steady = true;

    // Use non-saturating type here, because step may be fed into the standard library.
    static inline constexpr auto wait_step = std::chrono::hours(24);

    static time_point now() noexcept {
        auto time = std::chrono::steady_clock::now().time_since_epoch();
        return time_point(time);
    }

    template <typename Rep, typename Period>
    static void sleep_for(std::chrono::duration<Rep, Period> interval) {
        // Not using this_thread::sleep_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        return sleep_until(now() + interval);
    }

    template <typename Rep, typename Period>
    static void sleep_until(std::chrono::time_point<steady_clock, std::chrono::duration<Rep, Period>> until) {
        return sleep_until(std::chrono::time_point<steady_clock, std::chrono::duration<saturating<Rep>, Period>>(until));
    }

    template <typename Rep, typename Period>
    static void sleep_until(std::chrono::time_point<steady_clock, std::chrono::duration<saturating<Rep>, Period>> until) {
        // Implement in terms of repeated this_thread::sleep_for of non-"infinite" intervals.
        return internal::waitUntilViaFor(&now, wait_step, until, [&](auto interval) { std::this_thread::sleep_for(interval); });
    }
};

namespace test_support {

// Clock that is manually advanced. For testing purposes.
class manual_clock : public internal::ClockWaitImpl<manual_clock> {
public:
    using duration = nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<manual_clock>;

    static constexpr bool is_steady = false;

    // Use non-saturating type here, because step may be fed into the standard library.
    static inline constexpr auto wait_step = std::chrono::microseconds(1);

    static time_point now() noexcept { return now_.load(); }

    template <typename Rep, typename Period>
    static void sleep_for(std::chrono::duration<Rep, Period> interval) {
        sleep_until(now() + interval);
    }

    template <typename Rep, typename Period>
    static void sleep_until(std::chrono::time_point<manual_clock, std::chrono::duration<Rep, Period>> until) {
        return sleep_until(std::chrono::time_point<manual_clock, std::chrono::duration<saturating<Rep>, Period>>(until));
    }

    template <typename Rep, typename Period>
    static void sleep_until(std::chrono::time_point<manual_clock, std::chrono::duration<saturating<Rep>, Period>> until) {
        now_.store(until);
    }

private:
    static std::atomic<time_point> now_;
};

} // namespace test_support

} // namespace kotlin
