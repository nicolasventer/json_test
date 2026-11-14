#include "Timer.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

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
} // namespace

Timer::TaskHandle::~TaskHandle() { release(); }

void Timer::TaskHandle::release() const
{
	// Detach this handle, drop it from live_handles_, then cancel the task.
	if (timer_ == nullptr)
	{
		return;
	}

	Timer* const timer = timer_;
	const size_t id = id_;
	timer_ = nullptr;
	id_ = 0;

	timer->live_handles_.erase(
		std::remove(timer->live_handles_.begin(), timer->live_handles_.end(), this), timer->live_handles_.end());
	timer->cancel(id);
}

Timer::TaskHandle::TaskHandle(Timer& timer, size_t id) : timer_(&timer), id_(id) { timer.live_handles_.push_back(this); }

Timer::~Timer()
{
	// Clear live handles so they do not cancel into a destroyed timer.
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
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - real_origin_);
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
		// Jump virtual_now_ to the next due task or the target, then poll.
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
		// Sleep the thread in slices until the next task or the deadline.
		const auto deadline = std::chrono::steady_clock::now() + duration;
		const auto deadline_ms = now() + duration;
		for (;;)
		{
			const auto visible = std::min(now(), deadline_ms);
			if (timeUntilNext(visible).count() == 0)
			{
				pollAt(visible);
				continue;
			}

			const auto now_tp = std::chrono::steady_clock::now();
			if (now_tp >= deadline)
			{
				break;
			}

			const auto delay = timeUntilNext(visible);
			const auto remaining = deadline - now_tp;
			if (delay == std::chrono::milliseconds::max())
			{
				std::this_thread::sleep_for(remaining);
				continue;
			}

			const auto slice = std::min(std::chrono::duration_cast<std::chrono::steady_clock::duration>(delay), remaining);
			std::this_thread::sleep_for(slice);
		}
	}
}

void Timer::executeAt(std::chrono::milliseconds period, std::function<void(const TaskHandle&)> callback)
{
	// The timer stores the handle, so the task stays until cancel or destruction.
	period = normalizePeriod(period);

	const size_t id = next_id_++;
	auto handle = std::unique_ptr<TaskHandle>(new TaskHandle(*this, id));
	TaskHandle* const raw = handle.get();
	tasks_.push_back(Task{.id = id,
		.period = period,
		.next = now() + period,
		.fn = std::move(callback),
		.owned = std::move(handle),
		.handle = raw,
		.cancelled = false});
}

void Timer::executeAt(std::chrono::milliseconds period, std::function<void()> callback)
{
	executeAt(period,
		[callback = std::move(callback)](const TaskHandle&)
		{
			if (callback)
			{
				callback();
			}
		});
}

std::unique_ptr<Timer::TaskHandle> Timer::executeAtEx(std::chrono::milliseconds period, std::function<void()> callback)
{
	// The returned handle is not stored in the task. Destroying it cancels.
	period = normalizePeriod(period);

	const size_t id = next_id_++;
	auto handle = std::unique_ptr<TaskHandle>(new TaskHandle(*this, id));
	TaskHandle* const raw = handle.get();
	tasks_.push_back(Task{.id = id,
		.period = period,
		.next = now() + period,
		.fn =
			[callback = std::move(callback)](const TaskHandle&)
		{
			if (callback)
			{
				callback();
			}
		},
		.owned = std::unique_ptr<TaskHandle>{},
		.handle = raw,
		.cancelled = false});
	return handle;
}

std::chrono::milliseconds Timer::timeUntilNext(std::chrono::milliseconds current) const
{
	// Returns milliseconds::max() when nothing is scheduled, so real sleep waits out the whole remaining duration.
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

void Timer::poll() { pollAt(now()); }

void Timer::pollAt(std::chrono::milliseconds current)
{
	// Collect tasks that are due, push each next forward by period, invoke them, then reap cancelled tasks.
	for (;;)
	{
		std::vector<size_t> due;
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

		for (const size_t id : due)
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

void Timer::cancel(size_t id)
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
	tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(), [](const Task& task) { return task.cancelled; }), tasks_.end());
}

Timer::Task* Timer::find(size_t id)
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
