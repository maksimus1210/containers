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
     * \brief Извлечение элемента из контейнера.
     * \return элемент.
     */
    T pop();

    /**
     * \brief Ожидает изменения содержимого контейнера в течении заданного времени.
     * \param ms - максимальное время ожидания (мс).
     * \return если в контейнер не пуст возвращает true, иначе возвращает false.
     */
    bool wait(int64_t ms);

private:
    list<T> m_list;
    mutex   m_mutex;
    condition_variable m_condVar;
};

template <typename T>
void QueueConcurrent<T>::clear()
{
    // связываем блокировщик с переменной condition_variable
    unique_lock<mutex> t_locker(m_mutex);

    // очищаем контейнер
    m_list.clear();
}

template <typename T>
void QueueConcurrent<T>::push(T &&item)
{
    // связываем блокировщик с переменной condition_variable
    unique_lock<mutex> t_locker(m_mutex);

    // добавляем в контейнер новый элемент
    m_list.emplace_back(item);

    // уведомляем, что в содержимое контейнера изменилось
    m_condVar.notify_one();
}

template <typename T>
T QueueConcurrent<T>::pop()
{
    // связываем блокировщик с переменной condition_variable
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
bool QueueConcurrent<T>::wait(int64_t ms)
{ 
    // связываем блокировщик с переменной condition_variable
    unique_lock<mutex> t_locker(m_mutex);
       
    // возвращает true если контейнер не пуст, иначе входим в ожидание
    if (!m_list.empty())
        return true;

    // ожидаем срабатывания условной переменной по таймауту или событию notify_one()
    m_condVar.wait_until(t_locker, chrono::system_clock::now() + ms*1ms);

    // возвращаем состояние контейнера
    return !m_list.empty();
}

#endif // QUEUECONCURRENT_H
