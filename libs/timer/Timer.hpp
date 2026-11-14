#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

// Schedules repeating callbacks. Time is either virtual (advanced by sleep) or real (the steady clock).
// Tasks fire from sleep. Not copyable or movable.
// Destroying a TaskHandle cancels its task.
class Timer
{
public:
	// Cancels its task when destroyed or when release() is called.
	// executeAt keeps this internally; executeAtEx returns it to the caller.
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
		TaskHandle(Timer& timer, size_t id);

		mutable Timer* timer_ = nullptr;
		mutable size_t id_ = 0;
	};

	Timer() = default;
	~Timer();

	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;
	Timer(Timer&&) = delete;
	Timer& operator=(Timer&&) = delete;

	// Switches to virtual time and resets it to 0.
	void startVirtual();
	// Switches to real time. The origin is steady_clock::now().
	void startReal();

	// Elapsed virtual time, or time since startReal.
	// Stays 0 in Idle.
	[[nodiscard]] std::chrono::milliseconds now() const;

	// Advances virtual time, or blocks, until duration has passed, firing due tasks along the way.
	// No-op in Idle or when duration is negative.
	void sleep(std::chrono::milliseconds duration);

	// Repeating task owned by the timer. First due at now() + period.
	// The callback can cancel through the handle.
	// Periods of 0 or less become 1 ms.
	void executeAt(std::chrono::milliseconds period, std::function<void(const TaskHandle&)> callback);
	// Repeating task owned by the timer. First due at now() + period.
	// Periods of 0 or less become 1 ms.
	void executeAt(std::chrono::milliseconds period, std::function<void()> callback);

	// The caller owns the handle. The task lives until that handle is destroyed.
	// Ignoring the return value destroys the temporary at the end of the statement, which removes the callback immediately.
	[[nodiscard]] std::unique_ptr<TaskHandle> executeAtEx(std::chrono::milliseconds period, std::function<void()> callback);

private:
	enum class Mode
	{
		Idle,
		Virtual,
		Real
	};

	struct Task
	{
		size_t id = 0;
		std::chrono::milliseconds period{0};
		std::chrono::milliseconds next{0};
		std::function<void(const TaskHandle&)> fn;
		std::unique_ptr<TaskHandle> owned;
		TaskHandle* handle = nullptr;
		bool cancelled = false;
	};

	[[nodiscard]] std::chrono::milliseconds timeUntilNext(std::chrono::milliseconds current) const;
	void poll();
	void pollAt(std::chrono::milliseconds current);
	void cancel(size_t id);
	void reap();
	Task* find(size_t id);

	Mode mode_ = Mode::Idle;
	std::chrono::milliseconds virtual_now_{0};
	std::chrono::steady_clock::time_point real_origin_{};
	std::vector<Task> tasks_;
	std::vector<TaskHandle*> live_handles_;
	size_t next_id_ = 1;
};
