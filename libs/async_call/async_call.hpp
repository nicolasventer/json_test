#pragma once

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>

// Runs a callable on a background thread, one call at a time.
// try_get either returns the finished result or starts the next call.
// The destructor stops the worker and joins it. Not copyable or movable.
template <typename F> class AsyncCall
{
public:
	using result_type = std::invoke_result_t<F>;

	explicit AsyncCall(F function) : function_(std::move(function))
	{
		thread_ = std::thread([this] { run(); });
	}

	~AsyncCall()
	{
		{
			std::lock_guard lock(mutex_);
			stop_ = true;
		}
		cv_.notify_one();
		if (thread_.joinable())
		{
			thread_.join();
		}
	}

	AsyncCall(const AsyncCall&) = delete;
	AsyncCall& operator=(const AsyncCall&) = delete;
	AsyncCall(AsyncCall&&) = delete;
	AsyncCall& operator=(AsyncCall&&) = delete;

	// Non-blocking poll. Returns the finished call's value, or rethrows the exception it threw.
	// If nothing is ready, starts a call when none is in flight and returns nullopt.
	std::optional<result_type> try_get()
	{
		std::lock_guard lock(mutex_);
		if (ready_)
		{
			// Consume the finished call: rethrow its exception, or return its value.
			ready_ = false;
			if (error_)
			{
				std::exception_ptr error = error_;
				error_ = nullptr;
				std::rethrow_exception(error);
			}
			std::optional<result_type> result = std::move(result_);
			result_.reset();
			return result;
		}
		if (!running_)
		{
			// No call in flight: ask the worker to start one.
			running_ = true;
			start_ = true;
			cv_.notify_one();
		}
		return std::nullopt;
	}

private:
	// Worker loop. Waits until try_get requests a call or the destructor requests stop.
	// Invokes function_ outside the lock, then publishes the result or exception for the next try_get.
	void run()
	{
		std::unique_lock lock(mutex_);
		while (true)
		{
			// Wake when a call is requested (start_) or the destructor asks to stop.
			cv_.wait(lock, [this] { return stop_ || start_; });
			if (stop_)
			{
				// Destructor is tearing the call down.
				return;
			}
			start_ = false;
			lock.unlock();

			// Run the function without holding the mutex.
			std::optional<result_type> result;
			std::exception_ptr error;
			try
			{
				result = function_();
			}
			catch (...)
			{
				error = std::current_exception();
			}

			lock.lock();
			// Publish the result or exception so the next try_get can take it.
			result_ = std::move(result);
			error_ = error;
			running_ = false;
			ready_ = true;
		}
	}

	F function_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::optional<result_type> result_;
	std::exception_ptr error_;
	bool running_ = false;
	bool ready_ = false;
	bool start_ = false;
	bool stop_ = false;
	std::thread thread_;
};
