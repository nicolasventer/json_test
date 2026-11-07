#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

class Timer
{
public:
    class TaskHandle
    {
    public:
        ~TaskHandle();
        void release() const;

        TaskHandle(const TaskHandle&) = delete;
        TaskHandle& operator=(const TaskHandle&) = delete;
        TaskHandle(TaskHandle&&) = delete;
        TaskHandle& operator=(TaskHandle&&) = delete;

    private:
        friend class Timer;
        TaskHandle(Timer& timer, std::size_t id);

        mutable Timer* timer_ = nullptr;
        mutable std::size_t id_ = 0;
    };

    Timer() = default;
    ~Timer();

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    void startVirtual();
    void startReal();

    std::chrono::milliseconds now() const;

    void sleep(std::chrono::milliseconds duration);

    void executeAt(std::chrono::milliseconds period,
                   std::function<void(const TaskHandle&)> callback);
    void executeAt(std::chrono::milliseconds period, std::function<void()> callback);

    // Ignoring the return value destroys the temporary at the end of the
    // statement, which removes the callback immediately.
    [[nodiscard]] std::unique_ptr<TaskHandle> executeAtEx(
        std::chrono::milliseconds period,
        std::function<void()> callback);

private:
    enum class Mode
    {
        Idle,
        Virtual,
        Real
    };

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

    std::chrono::milliseconds timeUntilNext(std::chrono::milliseconds current) const;
    void poll();
    void pollAt(std::chrono::milliseconds current);
    void cancel(std::size_t id);
    void reap();
    Task* find(std::size_t id);

    Mode mode_ = Mode::Idle;
    std::chrono::milliseconds virtual_now_{0};
    std::chrono::steady_clock::time_point real_origin_{};
    std::vector<Task> tasks_;
    std::vector<TaskHandle*> live_handles_;
    std::size_t next_id_ = 1;
};
