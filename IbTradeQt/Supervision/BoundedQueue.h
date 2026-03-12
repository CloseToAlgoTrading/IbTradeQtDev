#ifndef SUPERVISION_BOUNDEDQUEUE_H
#define SUPERVISION_BOUNDEDQUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <chrono>

namespace Supervision {

enum class OverflowPolicy {
    DropOldest,
    DropNewest,
    Block
};

template<typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t maxSize, OverflowPolicy policy = OverflowPolicy::DropOldest)
        : m_maxSize(maxSize), m_policy(policy) {}

    bool push(T item) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_queue.size() >= m_maxSize) {
            switch (m_policy) {
                case OverflowPolicy::DropOldest:
                    m_queue.pop_front();
                    m_droppedCount++;
                    break;
                case OverflowPolicy::DropNewest:
                    m_droppedCount++;
                    return false;
                case OverflowPolicy::Block:
                    m_notFull.wait(lock, [this] {
                        return m_queue.size() < m_maxSize || !m_running;
                    });
                    if (!m_running) return false;
                    break;
            }
        }

        m_queue.push_back(std::move(item));
        m_notEmpty.notify_one();
        return true;
    }

    std::optional<T> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (!m_notEmpty.wait_for(lock, timeout, [this] {
            return !m_queue.empty() || !m_running;
        })) {
            return std::nullopt;
        }

        if (m_queue.empty()) return std::nullopt;

        T item = std::move(m_queue.front());
        m_queue.pop_front();
        m_notFull.notify_one();
        return item;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
        m_notEmpty.notify_all();
        m_notFull.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    size_t droppedCount() const { return m_droppedCount.load(); }
    bool isRunning() const { return m_running; }

private:
    size_t m_maxSize;
    OverflowPolicy m_policy;
    std::deque<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
    std::atomic<size_t> m_droppedCount{0};
    std::atomic<bool> m_running{true};
};

} // namespace Supervision

#endif // SUPERVISION_BOUNDEDQUEUE_H
