#ifndef QUEUECONCURRENT_H
#define QUEUECONCURRENT_H

#include <list>
#include <mutex>
#include <chrono>
#include <condition_variable>

using namespace std;

// проверялось на gcc 7.0.1 Kubuntu 17.04 x64
// с включённым c++17

template <typename T>
class QueueConcurrent
{
    using sys_clock = chrono::system_clock;
    using chrono_ms = chrono::milliseconds;

public:
    QueueConcurrent() = default;
    QueueConcurrent(const QueueConcurrent &) = delete;
    QueueConcurrent(QueueConcurrent &&) = delete;
    QueueConcurrent &operator=(const QueueConcurrent &) = delete;
    QueueConcurrent &operator=(QueueConcurrent &&) = delete;

    /**
     * \brief Очистка контейнера.
     */
    void clear();

    /**
     * \brief Добавление элемента в контейнер.
     * \param item - элемент.
     */
    void push(T &&item);

    /**
     * \brief Добавление элемента в контейнер.
     * \param item - элемент.
     */
    void push(const T &item);

    /**
     * \brief Извлечение элемента из контейнера.
     * \return элемент.
     */
    T pop();

    /**
     * \brief Попытка извлечь элемент из списка в течении заданного времени.
     * \param dstItem - элемент в который будет извлечён объект из контейнера.
     * \param ms - максимальное время ожидания (мс)
     */
    bool tryPop(T &dstItem, chrono_ms duration);

    /**
     * \brief вернёт true если контейнер пуст, иначе вернёт false.
     */
    bool empty();

private:
    list<T> m_list;
    mutex   m_mutex;
    condition_variable m_condVar;
};

template <typename T>
void QueueConcurrent<T>::clear()
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // очищаем контейнер
    m_list.clear();
}

template <typename T>
void QueueConcurrent<T>::push(T &&item)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // добавляем в контейнер новый элемент
    m_list.emplace_back(item);

    // уведомляем, что в содержимое контейнера изменилось
    m_condVar.notify_one();
}

template <typename T>
void QueueConcurrent<T>::push(const T &item)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // добавляем в контейнер новый элемент
    m_list.push_back(item);

    // уведомляем, что в содержимое контейнера изменилось
    m_condVar.notify_one();
}

template <typename T>
T QueueConcurrent<T>::pop()
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // ожидаем наполнения контейнера
    while (m_list.empty())
        m_condVar.wait(t_locker);

    // извлекаем элемент из контейнера
    auto item = m_list.front();
    m_list.pop_front();

    // снимаем блокировку
    t_locker.unlock();

    // возвращает элемент
    return std::move(item);
}

template <typename T>
bool QueueConcurrent<T>::tryPop(T &dstItem, chrono_ms duration)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // функция определения состояния контейнера
    auto cmp = [&](){ return !m_list.empty(); };

    // если контейнер пуст, то входим в ожидание
    if (!m_condVar.wait_until(t_locker, sys_clock::now() + duration, cmp))
        return false;

    // извлекаем элемент из контейнера
    dstItem = m_list.front();
    m_list.pop_front();

    // успешное выполнение
    return true;
}

template <typename T>
bool QueueConcurrent<T>::empty()
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // возвращаем состояние очереди
    return m_list.empty();
}

#endif // QUEUECONCURRENT_H
