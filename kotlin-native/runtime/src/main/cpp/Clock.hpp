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

using nanoseconds = std::chrono::duration<saturating<std::chrono::nanoseconds::rep>, std::chrono::nanoseconds::period>;
using microseconds = std::chrono::duration<saturating<std::chrono::microseconds::rep>, std::chrono::microseconds::period>;
using milliseconds = std::chrono::duration<saturating<std::chrono::milliseconds::rep>, std::chrono::milliseconds::period>;
using seconds = std::chrono::duration<saturating<std::chrono::seconds::rep>, std::chrono::seconds::period>;
using minutes = std::chrono::duration<saturating<std::chrono::minutes::rep>, std::chrono::minutes::period>;
using hours = std::chrono::duration<saturating<std::chrono::hours::rep>, std::chrono::hours::period>;

class steady_clock {
public:
    using rep = saturating<std::chrono::steady_clock::rep>;
    using period = std::chrono::steady_clock::period;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<steady_clock>;

    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        auto time = std::chrono::steady_clock::now().time_since_epoch();
        return time_point(time);
    }

    template <typename Rep, typename Period>
    static void sleep_for(std::chrono::duration<Rep, Period> interval) {
        // Not using this_thread::sleep_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        auto until = now() + interval;
        sleep_until(until);
    }

    template <typename Duration>
    static void sleep_until(std::chrono::time_point<steady_clock, Duration> until) {
        // Shield standard library from saturating types.
        auto untilNonSat = std::chrono::time_point<std::chrono::steady_clock>(until.time_since_epoch());
        std::this_thread::sleep_until(untilNonSat);
    }

    template <typename Rep, typename Period, typename F>
    static bool wait_for(
            std::condition_variable& cv, std::unique_lock<std::mutex>& lock, std::chrono::duration<Rep, Period> interval, F&& f) {
        // Not using cv.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        auto until = now() + interval;
        return wait_until(cv, lock, until, std::forward<F>(f));
    }

    template <typename Duration, typename F>
    static bool wait_until(
            std::condition_variable& cv, std::unique_lock<std::mutex>& lock, std::chrono::time_point<steady_clock, Duration> until, F&& f) {
        // Shield standard library from saturating types.
        auto untilNonSat = std::chrono::time_point<std::chrono::steady_clock>(until.time_since_epoch());
        return cv.wait_until(lock, untilNonSat, std::forward<F>(f));
    }

    template <typename Lock, typename Rep, typename Period, typename F>
    static bool wait_for(std::condition_variable_any& cv, Lock& lock, std::chrono::duration<Rep, Period> interval, F&& f) {
        // Not using cv.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        auto until = now() + interval;
        return wait_until(cv, lock, until, std::forward<F>(f));
    }

    template <typename Lock, typename Duration, typename F>
    static bool wait_until(std::condition_variable_any& cv, Lock& lock, std::chrono::time_point<steady_clock, Duration> until, F&& f) {
        // Shield standard library from saturating types.
        auto untilNonSat = std::chrono::time_point<std::chrono::steady_clock>(until.time_since_epoch());
        return cv.wait_until(lock, untilNonSat, std::forward<F>(f));
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_for(const std::future<T>& future, std::chrono::duration<Rep, Period> interval) {
        // Not using future.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        auto until = now() + interval;
        return wait_until(future, until);
    }

    template <typename T, typename Duration>
    static std::future_status wait_until(const std::future<T>& future, std::chrono::time_point<steady_clock, Duration> until) {
        // Shield standard library from saturating types.
        auto untilNonSat = std::chrono::time_point<std::chrono::steady_clock>(until.time_since_epoch());
        return future.wait_until(untilNonSat);
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_for(const std::shared_future<T>& future, std::chrono::duration<Rep, Period> interval) {
        // Not using future.wait_for, because it may mishandle "infinite" intervals. Use saturating arithmetics to address this.
        auto until = now() + interval;
        return wait_until(future, until);
    }

    template <typename T, typename Duration>
    static std::future_status wait_until(const std::shared_future<T>& future, std::chrono::time_point<steady_clock, Duration> until) {
        // Shield standard library from saturating types.
        auto untilNonSat = std::chrono::time_point<std::chrono::steady_clock>(until.time_since_epoch());
        return future.wait_until(untilNonSat);
    }
};

namespace test_support {

// Clock that is manually advanced. For testing purposes.
class manual_clock {
public:
    using duration = nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<manual_clock>;

    static constexpr bool is_steady = false;

    static time_point now() noexcept { return now_.load(); }

    template <typename Rep, typename Period>
    static void sleep_for(std::chrono::duration<Rep, Period> interval) {
        auto until = now() + interval;
        sleep_until(until);
    }

    template <typename Duration>
    static void sleep_until(std::chrono::time_point<manual_clock, Duration> until) {
        now_.store(until);
    }

    template <typename Rep, typename Period, typename F>
    static bool wait_for(
            std::condition_variable& cv, std::unique_lock<std::mutex>& lock, std::chrono::duration<Rep, Period> interval, F&& f) {
        auto until = now() + interval;
        return wait_until(cv, lock, until, std::forward<F>(f));
    }

    template <typename Duration, typename F>
    static bool wait_until(
            std::condition_variable& cv, std::unique_lock<std::mutex>& lock, std::chrono::time_point<manual_clock, Duration> until, F&& f) {
        while (true) {
            if (now_.load() >= until) {
                return false;
            }
            // Release the lock and wait a bit to allow other threads to either advance time or change the predicate result.
            auto result = cv.wait_for(lock, std::chrono::microseconds(1), std::forward<F>(f));
            if (result) {
                return true;
            }
        }
    }

    template <typename Lock, typename Rep, typename Period, typename F>
    static bool wait_for(std::condition_variable_any& cv, Lock& lock, std::chrono::duration<Rep, Period> interval, F&& f) {
        auto until = now() + interval;
        return wait_until(cv, lock, until, std::forward<F>(f));
    }

    template <typename Lock, typename Duration, typename F>
    static bool wait_until(std::condition_variable_any& cv, Lock& lock, std::chrono::time_point<manual_clock, Duration> until, F&& f) {
        while (true) {
            if (now_.load() >= until) {
                return false;
            }
            // Release the lock and wait a bit to allow other threads to either advance time or change the predicate result.
            auto result = cv.wait_for(lock, std::chrono::microseconds(1), std::forward<F>(f));
            if (result) {
                return true;
            }
        }
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_for(std::future<T>& future, std::chrono::duration<Rep, Period> interval) {
        auto until = now() + interval;
        return wait_until(future, until);
    }

    template <typename T, typename Duration>
    static std::future_status wait_until(std::future<T>& future, std::chrono::time_point<manual_clock, Duration> until) {
        while (true) {
            if (now_.load() >= until) {
                return std::future_status::timeout;
            }
            // Wait a bit to allow other threads to either advance time or change the future result.
            auto result = future.wait_for(std::chrono::microseconds(1));
            if (result != std::future_status::timeout) {
                return result;
            }
        }
    }

    template <typename T, typename Rep, typename Period>
    static std::future_status wait_for(std::shared_future<T>& future, std::chrono::duration<Rep, Period> interval) {
        auto until = now() + interval;
        return wait_until(future, until);
    }

    template <typename T, typename Duration>
    static std::future_status wait_until(std::shared_future<T>& future, std::chrono::time_point<manual_clock, Duration> until) {
        while (true) {
            if (now_.load() >= until) {
                return std::future_status::timeout;
            }
            // Wait a bit to allow other threads to either advance time or change the future result.
            auto result = future.wait_for(std::chrono::microseconds(1));
            if (result != std::future_status::timeout) {
                return result;
            }
        }
    }

private:
    static std::atomic<time_point> now_;
};

} // namespace test_support

} // namespace kotlin
