#pragma once

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>

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

	std::optional<result_type> try_get()
	{
		std::lock_guard lock(mutex_);
		if (ready_)
		{
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
			running_ = true;
			start_ = true;
			cv_.notify_one();
		}
		return std::nullopt;
	}

private:
	void run()
	{
		std::unique_lock lock(mutex_);
		while (true)
		{
			cv_.wait(lock, [this] { return stop_ || start_; });
			if (stop_)
			{
				return;
			}
			start_ = false;
			lock.unlock();

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
