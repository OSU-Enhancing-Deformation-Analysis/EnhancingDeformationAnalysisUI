#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
      public:
	// Add a task to the thread pool
	template <class F, class... Args>
	auto enqueue(F &&f, Args &&...args)
	    -> std::future<typename std::invoke_result<F, Args...>::type>;

	static ThreadPool &GetThreadPool() {
		static ThreadPool instance;
		return instance;
	}

	// Get the number of tasks in the queue
	size_t get_queue_size();

	// Set a callback to be called when a task completes
	void set_on_task_complete(std::function<void()> callback);

	// Get the number of active tasks
	size_t get_active_tasks() const;

      private:
	// private because we want to use the singleton pattern
	ThreadPool(size_t num_threads = 0);
	~ThreadPool();

	std::vector<std::thread> m_workers;
	std::queue<std::function<void()>> m_tasks;

	std::mutex m_queue_mutex;
	std::condition_variable m_condition;
	std::atomic<bool> m_stop;
	std::atomic<size_t> m_active_tasks;
	std::function<void()> m_on_task_complete;
};

// Implementation of the enqueue function
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {

	using return_type = typename std::invoke_result<F, Args...>::type;

	auto task = std::make_shared<std::packaged_task<return_type()>>(
	    std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	std::future<return_type> res = task->get_future();

	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);

		// Don't allow enqueueing after stopping the pool
		if (m_stop) {
			throw std::runtime_error(
			    "Enqueue on stopped ThreadPool");
		}

		m_tasks.emplace([this, task]() {
			m_active_tasks++;
			(*task)();
			m_active_tasks--;

			if (m_on_task_complete) {
				m_on_task_complete();
			}
		});
	}

	m_condition.notify_one();
	return res;
}
