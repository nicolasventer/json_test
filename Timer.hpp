#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

class Timer
{
public:
    Timer() = default;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    void startVirtual()
    {
        mode_ = Mode::Virtual;
        virtual_now_ = std::chrono::milliseconds{0};
    }

    void startReal()
    {
        mode_ = Mode::Real;
        real_origin_ = std::chrono::steady_clock::now();
    }

    std::chrono::milliseconds now() const
    {
        if (mode_ == Mode::Real)
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - real_origin_);
        }

        return virtual_now_;
    }

    void sleep(std::chrono::milliseconds duration);

    std::size_t addOnSleep(std::function<void(std::size_t)> callback);
    std::size_t addOnSleep(std::function<void()> callback);
    void removeOnSleep(std::size_t id);

    class Executor
    {
    public:
        std::size_t executeAt(std::chrono::milliseconds period, std::function<void(std::size_t)> callback);
        std::size_t executeAt(std::chrono::milliseconds period, std::function<void()> callback);
        void stopExecute(std::size_t id);

    private:
        friend class Timer;

        Executor(Timer& timer, std::chrono::milliseconds period);

        bool hasTasks() const { return !tasks_.empty(); }

        std::chrono::milliseconds timeUntilCheck(std::chrono::milliseconds current) const
        {
            const auto next = last_check_ + check_period_;
            if (next <= current)
            {
                return std::chrono::milliseconds{0};
            }
            return next - current;
        }

        bool contains(std::size_t id) const
        {
            for (const Task& task : tasks_)
            {
                if (task.id == id)
                {
                    return true;
                }
            }
            return false;
        }

        void poll();

        struct Task
        {
            std::size_t id = 0;
            std::chrono::milliseconds period{0};
            std::chrono::milliseconds next{0};
            std::function<void(std::size_t)> fn;
        };

        Timer& timer_;
        std::chrono::milliseconds check_period_;
        std::chrono::milliseconds last_check_{0};
        std::vector<Task> tasks_;
        std::size_t next_id_ = 1;
    };

    Executor& getExecutor(std::chrono::milliseconds period);

private:
    enum class Mode
    {
        Idle,
        Virtual,
        Real
    };

    std::chrono::milliseconds nextCheckDelay() const;
    void pollDueExecutors();
    void notifyOnSleep();

    Mode mode_ = Mode::Idle;
    std::chrono::milliseconds virtual_now_{0};
    std::chrono::steady_clock::time_point real_origin_{};

    struct SleepHook
    {
        std::size_t id = 0;
        std::function<void(std::size_t)> fn;
    };

    std::vector<SleepHook> on_sleep_;
    std::size_t next_sleep_id_ = 1;
    std::map<std::chrono::milliseconds, std::unique_ptr<Executor>> executors_;
};

inline void Timer::sleep(std::chrono::milliseconds duration)
{
    if (mode_ == Mode::Idle || duration.count() < 0)
    {
        return;
    }

    if (mode_ == Mode::Virtual)
    {
        const auto target = virtual_now_ + duration;
        while (virtual_now_ < target)
        {
            const auto delay = nextCheckDelay();
            const auto remaining = target - virtual_now_;
            if (delay.count() == 0)
            {
                pollDueExecutors();
                continue;
            }
            if (delay < remaining)
            {
                virtual_now_ += delay;
                pollDueExecutors();
                continue;
            }
            virtual_now_ = target;
        }
    }
    else
    {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline)
        {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            if (remaining.count() <= 0)
            {
                break;
            }

            const auto delay = nextCheckDelay();
            if (delay.count() == 0)
            {
                pollDueExecutors();
                continue;
            }

            const bool check_due = delay <= remaining;
            const auto slice = check_due ? delay : remaining;
            std::this_thread::sleep_for(slice);
            if (check_due)
            {
                pollDueExecutors();
            }
        }
    }

    notifyOnSleep();
}

inline std::size_t Timer::addOnSleep(std::function<void(std::size_t)> callback)
{
    const std::size_t id = next_sleep_id_++;
    on_sleep_.push_back(SleepHook{id, std::move(callback)});
    return id;
}

inline std::size_t Timer::addOnSleep(std::function<void()> callback)
{
    return addOnSleep([callback = std::move(callback)](std::size_t) {
        if (callback)
        {
            callback();
        }
    });
}

inline void Timer::removeOnSleep(std::size_t id)
{
    on_sleep_.erase(
        std::remove_if(on_sleep_.begin(), on_sleep_.end(),
                       [id](const SleepHook& hook) { return hook.id == id; }),
        on_sleep_.end());
}

inline Timer::Executor::Executor(Timer& timer, std::chrono::milliseconds period)
    : timer_(timer), check_period_(period), last_check_(timer.now())
{
}

inline std::size_t Timer::Executor::executeAt(std::chrono::milliseconds period,
                                             std::function<void(std::size_t)> callback)
{
    if (period.count() <= 0)
    {
        period = check_period_.count() > 0 ? check_period_ : std::chrono::milliseconds{1};
    }

    const std::size_t id = next_id_++;
    tasks_.push_back(Task{id, period, timer_.now() + period, std::move(callback)});
    return id;
}

inline std::size_t Timer::Executor::executeAt(std::chrono::milliseconds period, std::function<void()> callback)
{
    return executeAt(period, [callback = std::move(callback)](std::size_t) {
        if (callback)
        {
            callback();
        }
    });
}

inline void Timer::Executor::stopExecute(std::size_t id)
{
    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(),
                                [id](const Task& task) { return task.id == id; }),
                 tasks_.end());
}

inline void Timer::Executor::poll()
{
    const auto current = timer_.now();
    for (;;)
    {
        std::vector<Task> due;
        for (Task& task : tasks_)
        {
            if (task.next <= current)
            {
                due.push_back(task);
                task.next += task.period;
            }
        }
        if (due.empty())
        {
            break;
        }

        for (const Task& task : due)
        {
            if (!contains(task.id) || !task.fn)
            {
                continue;
            }
            task.fn(task.id);
        }
    }
    last_check_ = current;
}

inline Timer::Executor& Timer::getExecutor(std::chrono::milliseconds period)
{
    if (period.count() <= 0)
    {
        period = std::chrono::milliseconds{1};
    }

    const auto found = executors_.find(period);
    if (found != executors_.end())
    {
        return *found->second;
    }

    auto executor = std::unique_ptr<Executor>(new Executor(*this, period));
    Executor& ref = *executor;
    executors_.emplace(period, std::move(executor));
    return ref;
}

inline std::chrono::milliseconds Timer::nextCheckDelay() const
{
    const auto current = now();
    bool found = false;
    std::chrono::milliseconds earliest{0};
    for (const auto& entry : executors_)
    {
        const Executor& executor = *entry.second;
        if (!executor.hasTasks())
        {
            continue;
        }
        const auto due_in = executor.timeUntilCheck(current);
        if (!found || due_in < earliest)
        {
            earliest = due_in;
            found = true;
        }
    }
    if (!found)
    {
        return std::chrono::milliseconds::max();
    }
    return earliest;
}

inline void Timer::pollDueExecutors()
{
    const auto current = now();
    for (auto& entry : executors_)
    {
        Executor& executor = *entry.second;
        if (!executor.hasTasks())
        {
            continue;
        }
        if (executor.timeUntilCheck(current).count() == 0)
        {
            executor.poll();
        }
    }
}

inline void Timer::notifyOnSleep()
{
    const std::vector<SleepHook> hooks = on_sleep_;
    for (const SleepHook& hook : hooks)
    {
        bool still_there = false;
        for (const SleepHook& current : on_sleep_)
        {
            if (current.id == hook.id)
            {
                still_there = true;
                break;
            }
        }
        if (!still_there || !hook.fn)
        {
            continue;
        }
        hook.fn(hook.id);
    }
}
