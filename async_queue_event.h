#ifndef ASYNC_QUEUE_EVENT_H
#define ASYNC_QUEUE_EVENT_H

#include <atomic>
#include <QtCore>

using namespace std;

template <typename T>
class AsyncQueueEvent
{
public:
    AsyncQueueEvent();
    ~AsyncQueueEvent();

    void clear();

    quint32 count() const noexcept;

    void push(T &data);
    bool pop(T &data);

    bool waitForReadyRead(quint32 timeoutMs);

private:
    AsyncQueueEvent(const AsyncQueueEvent &) = delete;
    AsyncQueueEvent &operator=(const AsyncQueueEvent &) = delete;

    inline void createEvent();
    inline void setEvent();
    inline void resetEvent();
    inline bool waitEvent(quint32 timeoutMs);

private:
    atomic<quint32> m_count;
    atomic<quint32> m_eventCnt;
    QSemaphore      m_semaphore;
    QQueue<T>       m_queue;
    QMutex          m_mutex;
};

template <typename T>
AsyncQueueEvent<T>::AsyncQueueEvent() :
  m_count(0u),
  m_eventCnt(0u),
  m_semaphore(1)
{
    createEvent();
}

template <typename T>
AsyncQueueEvent<T>::~AsyncQueueEvent()
{
    m_semaphore.tryAcquire(1, 1);
    m_semaphore.release(1);
}

template <typename T>
quint32 AsyncQueueEvent<T>::count() const noexcept
{
    return m_count;
}

template <typename T>
void AsyncQueueEvent<T>::push(T &data)
{
    m_mutex.lock();
        m_queue.enqueue(data);
        ++m_count;
    m_mutex.unlock();

    setEvent();
}

template <typename T>
void AsyncQueueEvent<T>::clear()
{
    T t_temp;
    while (pop(t_temp));
}

template <typename T>
bool AsyncQueueEvent<T>::pop(T &data)
{
    if (m_count < 1u)
        return false;

    if (--m_count == 0u)
        resetEvent();

    m_mutex.lock();
        data = m_queue.dequeue();
    m_mutex.unlock();
    return true;
}

template <typename T>
bool AsyncQueueEvent<T>::waitForReadyRead(quint32 timeoutMs)
{
    if (m_count > 0u)
        return true;

    return waitEvent(timeoutMs);
}

template <typename T>
void AsyncQueueEvent<T>::createEvent()
{
    m_eventCnt = 0u;
    m_semaphore.acquire(1);
}

template <typename T>
void AsyncQueueEvent<T>::setEvent()
{
    ++m_eventCnt;
    m_semaphore.release(1);
}

template <typename T>
bool AsyncQueueEvent<T>::waitEvent(quint32 timeoutMs)
{
    if (m_count > 0u)
        return true;

    return m_semaphore.tryAcquire(1, timeoutMs);
}

template <typename T>
void AsyncQueueEvent<T>::resetEvent()
{
    if (m_eventCnt == 0u)
        m_semaphore.acquire(1);
    else
        --m_eventCnt;
}

#endif // ASYNC_QUEUE_EVENT_H
