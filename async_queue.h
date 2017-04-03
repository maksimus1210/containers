#ifndef ASYNC_QUEUE_H
#define ASYNC_QUEUE_H

#include <QMutex>
#include <QQueue>

template<class T> class AsyncQueue
{
public:
    AsyncQueue()
    {
    }

    ~AsyncQueue()
    {
        clean();
    }

    uint count()
    {
        m_mutex.lock();
            int count = m_queue.count();
        m_mutex.unlock();
        return count;
    }

    bool avalible()
    {
        m_mutex.lock();
            int count = m_queue.count();
        m_mutex.unlock();
        return count;
    }

    bool isEmpty()
    {
        m_mutex.lock();
            bool empty = m_queue.isEmpty();
        m_mutex.unlock();
        return empty;
    }

    void clean()
    {
        m_mutex.lock();
            m_queue.clear();
        m_mutex.unlock();
    }

    void push(const T& t)
    {
        m_mutex.lock();
            m_queue.enqueue(t);
        m_mutex.unlock();
    }

    T pull()
    {
        m_mutex.lock();
            T i = m_queue.dequeue();
        m_mutex.unlock();
        return i;
    }

private:
    QQueue<T> m_queue;
    QMutex    m_mutex;
};

#endif // ASYNC_QUEUE_H
