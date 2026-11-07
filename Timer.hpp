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
        class TaskHandle
        {
        public:
            ~TaskHandle();
            void release() const;

            TaskHandle(const TaskHandle&) = delete;
            TaskHandle& operator=(const TaskHandle&) = delete;

        private:
            friend class Executor;
            TaskHandle(Executor& executor, std::size_t id);

            mutable Executor* executor_ = nullptr;
            mutable std::size_t id_ = 0;
        };

        ~Executor();

        void executeAt(std::chrono::milliseconds period,
                       std::function<void(const TaskHandle&)> callback);
        void executeAt(std::chrono::milliseconds period, std::function<void()> callback);

        // Ignoring the return value destroys the temporary at the end of the
        // statement, which removes the callback immediately.
        [[nodiscard]] std::unique_ptr<TaskHandle> executeAtEx(
            std::chrono::milliseconds period,
            std::function<void()> callback);

    private:
        friend class Timer;

        Executor(Timer& timer, std::chrono::milliseconds period);

        bool hasTasks() const;

        std::chrono::milliseconds timeUntilCheck(std::chrono::milliseconds current) const;

        void poll();
        void cancel(std::size_t id);
        void reap();

        struct Task
        {
            std::size_t id = 0;
            std::chrono::milliseconds period{0};
            std::chrono::milliseconds next{0};
            std::function<void(const TaskHandle&)> fn;
            std::unique_ptr<TaskHandle> owned;
            TaskHandle* handle = nullptr;
            bool cancelled = false;
        };

        Task* find(std::size_t id);

        Timer& timer_;
        std::chrono::milliseconds check_period_;
        std::chrono::milliseconds last_check_{0};
        std::vector<Task> tasks_;
        std::vector<TaskHandle*> live_handles_;
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
