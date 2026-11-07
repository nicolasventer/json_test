#include "Timer.hpp"

#include <algorithm>
#include <thread>
#include <utility>

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
            const auto delay = nextCheckDelay();
            const auto remaining = target - virtual_now_;
            if (delay.count() == 0)
            {
                pollDueExecutors();
                continue;
            }
            if (delay <= remaining)
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

std::size_t Timer::addOnSleep(std::function<void(std::size_t)> callback)
{
    const std::size_t id = next_sleep_id_++;
    on_sleep_.push_back(SleepHook{id, std::move(callback)});
    return id;
}

std::size_t Timer::addOnSleep(std::function<void()> callback)
{
    return addOnSleep([callback = std::move(callback)](std::size_t) {
        if (callback)
        {
            callback();
        }
    });
}

void Timer::removeOnSleep(std::size_t id)
{
    on_sleep_.erase(
        std::remove_if(on_sleep_.begin(), on_sleep_.end(),
                       [id](const SleepHook& hook) { return hook.id == id; }),
        on_sleep_.end());
}

Timer::Executor::TaskHandle::TaskHandle(Executor& executor, std::size_t id)
    : executor_(&executor), id_(id)
{
    executor.live_handles_.push_back(this);
}

Timer::Executor::TaskHandle::~TaskHandle()
{
    release();
}

void Timer::Executor::TaskHandle::release() const
{
    if (executor_ == nullptr)
    {
        return;
    }

    Executor* const exec = executor_;
    const std::size_t id = id_;
    executor_ = nullptr;
    id_ = 0;

    exec->live_handles_.erase(
        std::remove(exec->live_handles_.begin(), exec->live_handles_.end(), this),
        exec->live_handles_.end());
    exec->cancel(id);
}

Timer::Executor::Executor(Timer& timer, std::chrono::milliseconds period)
    : timer_(timer), check_period_(period), last_check_(timer.now())
{
}

Timer::Executor::~Executor()
{
    for (TaskHandle* handle : live_handles_)
    {
        if (handle == nullptr)
        {
            continue;
        }
        handle->executor_ = nullptr;
        handle->id_ = 0;
    }
    live_handles_.clear();
    tasks_.clear();
}

bool Timer::Executor::hasTasks() const
{
    for (const Task& task : tasks_)
    {
        if (!task.cancelled)
        {
            return true;
        }
    }
    return false;
}

std::chrono::milliseconds Timer::Executor::timeUntilCheck(std::chrono::milliseconds current) const
{
    const auto next = last_check_ + check_period_;
    if (next <= current)
    {
        return std::chrono::milliseconds{0};
    }
    return next - current;
}

Timer::Executor::Task* Timer::Executor::find(std::size_t id)
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

void Timer::Executor::cancel(std::size_t id)
{
    Task* const task = find(id);
    if (task == nullptr)
    {
        return;
    }
    task->cancelled = true;
    task->fn = nullptr;
}

void Timer::Executor::reap()
{
    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(),
                                [](const Task& task) { return task.cancelled; }),
                 tasks_.end());
}

namespace
{
std::chrono::milliseconds normalizePeriod(std::chrono::milliseconds period,
                                          std::chrono::milliseconds check_period)
{
    if (period.count() <= 0)
    {
        return check_period.count() > 0 ? check_period : std::chrono::milliseconds{1};
    }
    return period;
}
}  // namespace

void Timer::Executor::executeAt(std::chrono::milliseconds period,
                                std::function<void(const TaskHandle&)> callback)
{
    period = normalizePeriod(period, check_period_);

    const std::size_t id = next_id_++;
    auto handle = std::unique_ptr<TaskHandle>(new TaskHandle(*this, id));
    TaskHandle* const raw = handle.get();
    tasks_.push_back(Task{id, period, timer_.now() + period, std::move(callback), std::move(handle), raw, false});
}

void Timer::Executor::executeAt(std::chrono::milliseconds period, std::function<void()> callback)
{
    executeAt(period, [callback = std::move(callback)](const TaskHandle&) {
        if (callback)
        {
            callback();
        }
    });
}

std::unique_ptr<Timer::Executor::TaskHandle> Timer::Executor::executeAtEx(
    std::chrono::milliseconds period,
    std::function<void()> callback)
{
    period = normalizePeriod(period, check_period_);

    const std::size_t id = next_id_++;
    auto handle = std::unique_ptr<TaskHandle>(new TaskHandle(*this, id));
    TaskHandle* const raw = handle.get();
    tasks_.push_back(Task{
        id,
        period,
        timer_.now() + period,
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

void Timer::Executor::poll()
{
    const auto current = timer_.now();
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
    last_check_ = current;
}

Timer::Executor& Timer::getExecutor(std::chrono::milliseconds period)
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

std::chrono::milliseconds Timer::nextCheckDelay() const
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

void Timer::pollDueExecutors()
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

void Timer::notifyOnSleep()
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
