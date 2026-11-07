#include "Timer.hpp"

#include <algorithm>
#include <thread>
#include <utility>

namespace
{
std::chrono::milliseconds normalizePeriod(std::chrono::milliseconds period)
{
    if (period.count() <= 0)
    {
        return std::chrono::milliseconds{1};
    }
    return period;
}
}  // namespace

Timer::TaskHandle::~TaskHandle()
{
    release();
}

void Timer::TaskHandle::release() const
{
    if (timer_ == nullptr)
    {
        return;
    }

    Timer* const timer = timer_;
    const std::size_t id = id_;
    timer_ = nullptr;
    id_ = 0;

    timer->live_handles_.erase(
        std::remove(timer->live_handles_.begin(), timer->live_handles_.end(), this),
        timer->live_handles_.end());
    timer->cancel(id);
}

Timer::TaskHandle::TaskHandle(Timer& timer, std::size_t id)
    : timer_(&timer), id_(id)
{
    timer.live_handles_.push_back(this);
}

Timer::~Timer()
{
    for (TaskHandle* handle : live_handles_)
    {
        if (handle == nullptr)
        {
            continue;
        }
        handle->timer_ = nullptr;
        handle->id_ = 0;
    }
    live_handles_.clear();
    tasks_.clear();
}

void Timer::startVirtual()
{
    mode_ = Mode::Virtual;
    virtual_now_ = std::chrono::milliseconds{0};
}

void Timer::startReal()
{
    mode_ = Mode::Real;
    real_origin_ = std::chrono::steady_clock::now();
}

std::chrono::milliseconds Timer::now() const
{
    if (mode_ == Mode::Real)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - real_origin_);
    }

    return virtual_now_;
}

void Timer::sleep(std::chrono::milliseconds duration)
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
            const auto delay = timeUntilNext(virtual_now_);
            const auto remaining = target - virtual_now_;
            if (delay.count() == 0)
            {
                poll();
                continue;
            }
            if (delay <= remaining)
            {
                virtual_now_ += delay;
                poll();
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

            const auto delay = timeUntilNext(now());
            if (delay.count() == 0)
            {
                poll();
                continue;
            }

            const bool due = delay <= remaining;
            const auto slice = due ? delay : remaining;
            std::this_thread::sleep_for(slice);
            if (due)
            {
                poll();
            }
        }
    }
}

void Timer::executeAt(std::chrono::milliseconds period,
                      std::function<void(const TaskHandle&)> callback)
{
    period = normalizePeriod(period);

    const std::size_t id = next_id_++;
    auto handle = std::unique_ptr<TaskHandle>(new TaskHandle(*this, id));
    TaskHandle* const raw = handle.get();
    tasks_.push_back(Task{id, period, now() + period, std::move(callback), std::move(handle), raw, false});
}

void Timer::executeAt(std::chrono::milliseconds period, std::function<void()> callback)
{
    executeAt(period, [callback = std::move(callback)](const TaskHandle&) {
        if (callback)
        {
            callback();
        }
    });
}

std::unique_ptr<Timer::TaskHandle> Timer::executeAtEx(std::chrono::milliseconds period,
                                                      std::function<void()> callback)
{
    period = normalizePeriod(period);

    const std::size_t id = next_id_++;
    auto handle = std::unique_ptr<TaskHandle>(new TaskHandle(*this, id));
    TaskHandle* const raw = handle.get();
    tasks_.push_back(Task{
        id,
        period,
        now() + period,
        [callback = std::move(callback)](const TaskHandle&) {
            if (callback)
            {
                callback();
            }
        },
        std::unique_ptr<TaskHandle>{},
        raw,
        false});
    return handle;
}

std::chrono::milliseconds Timer::timeUntilNext(std::chrono::milliseconds current) const
{
    bool found = false;
    std::chrono::milliseconds earliest{0};
    for (const Task& task : tasks_)
    {
        if (task.cancelled)
        {
            continue;
        }
        const auto due_in = task.next <= current ? std::chrono::milliseconds{0} : task.next - current;
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

void Timer::poll()
{
    const auto current = now();
    for (;;)
    {
        std::vector<std::size_t> due;
        for (Task& task : tasks_)
        {
            if (task.cancelled)
            {
                continue;
            }
            if (task.next <= current)
            {
                due.push_back(task.id);
                task.next += task.period;
            }
        }
        if (due.empty())
        {
            break;
        }

        for (const std::size_t id : due)
        {
            Task* const task = find(id);
            if (task == nullptr || task->cancelled || !task->fn || task->handle == nullptr)
            {
                continue;
            }

            const std::function<void(const TaskHandle&)> fn = task->fn;
            TaskHandle& handle = *task->handle;
            fn(handle);
        }
    }

    reap();
}

void Timer::cancel(std::size_t id)
{
    Task* const task = find(id);
    if (task == nullptr)
    {
        return;
    }
    task->cancelled = true;
    task->fn = nullptr;
}

void Timer::reap()
{
    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(),
                                [](const Task& task) { return task.cancelled; }),
                 tasks_.end());
}

Timer::Task* Timer::find(std::size_t id)
{
    for (Task& task : tasks_)
    {
        if (task.id == id)
        {
            return &task;
        }
    }
    return nullptr;
}
