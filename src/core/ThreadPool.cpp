#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t num_threads) : m_stop(false), m_active_tasks(0) {

	// Use hardware concurrency if num_threads is 0
	if (num_threads == 0) {
		num_threads = std::thread::hardware_concurrency();
		// Use at least 2 threads
		if (num_threads <= 1) {
			num_threads = 2;
		}
	}

	for (size_t i = 0; i < num_threads; ++i) {
		m_workers.emplace_back([this] {
			while (true) {
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(
					    m_queue_mutex);

					// Wait until there's a task or the pool
					// is stopped
					m_condition.wait(lock, [this] {
						return m_stop ||
						       !m_tasks.empty();
					});

					// Exit if the pool is stopped and there
					// are no more tasks
					if (m_stop && m_tasks.empty()) {
						return;
					}

					// Get the next task
					task = std::move(m_tasks.front());
					m_tasks.pop();
				}

				// Execute the task
				task();
			}
		});
	}
}

ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		m_stop = true;
	}

	// Wake up all worker threads
	m_condition.notify_all();

	// Join all threads
	for (std::thread &worker : m_workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

size_t ThreadPool::get_queue_size() {
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	return m_tasks.size();
}

void ThreadPool::set_on_task_complete(std::function<void()> callback) {
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	m_on_task_complete = callback;
}

size_t ThreadPool::get_active_tasks() const { return m_active_tasks; }
