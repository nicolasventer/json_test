#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <vector>

class Timer
{
public:
    Timer() = default;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    void startVirtual();
    void startReal();

    std::chrono::milliseconds now() const;

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

        bool hasTasks() const;

        std::chrono::milliseconds timeUntilCheck(std::chrono::milliseconds current) const;

        bool contains(std::size_t id) const;

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
