#ifndef RINGBUFFERCONCURRENT_H
#define RINGBUFFERCONCURRENT_H

#include <vector>
#include <mutex>
#include <chrono>
#include <condition_variable>

using namespace std;

template <typename T>
class RingBufferConcurrent
{
    using sys_clock = chrono::system_clock;
    using chrono_ms = chrono::milliseconds;

public:
    /**
     * \brief Конструктор класса по умолчанию.
     * \param t_size - желаемый размер буфера.
     *
     * \details Размер буфера может быть только кратным степени двойки.
     */
    explicit RingBufferConcurrent(size_t t_size = 0);

    RingBufferConcurrent(const RingBufferConcurrent &) = delete;
    RingBufferConcurrent(RingBufferConcurrent &&) = delete;
    RingBufferConcurrent &operator=(const RingBufferConcurrent &) = delete;
    RingBufferConcurrent &operator=(RingBufferConcurrent &&) = delete;


    /**
     * \brief Изменение размера буфера.
     * \param t_size - желаемый размер буфера.
     */
    void resize(size_t t_size);

    /**
     * \brief Возвращает реальный размер буфера.
     */
    size_t size() const;


    /**
     * \brief Удаление записанных данных.
     */
    void clear();


    /**
     * \brief Запись данных в буфер.
     * \param items - вектор элементов.
     */
    void write(const vector<T> &items);

    /**
     * \brief Чтение данных из буфера размером dst.size().
     * \param items - вектор куда будут записаны данные.
     */
    void read(vector<T> &dst);


    /**
     * \brief Попытка записать данные в течении заданного интервала времени.
     * \param items - вектор элементов.
     * \param duration - интервал времени (можно использовать литералы 200sm, 1s итд).
     * \return если данные записаны, то вернёт true иначе вернёт false.
     */
    bool tryWrite(const vector<T> &items, const chrono_ms duration);

    /**
     * \brief Попытка прочитать данные размером dst.size() в течении заданного интервала времени.
     * \param items - вектор куда будут записаны данные.
     * \param duration - интервал времени (можно использовать литералы 200sm, 1s итд).
     * \return если данные прочитаны, то вернёт true иначе вернёт false.
     */
    bool tryRead(vector<T> &dst, const chrono_ms duration);

private:
    constexpr size_t pow2Next(size_t d) const noexcept
    {
        if (d != 0) {
            --d;
            d |= d >> 1;
            d |= d >> 2;
            d |= d >> 4;
            d |= d >> 8;
            d |= d >> 16;
            ++d;
            return d;
        }

        return 0;
    }

private:
    vector<T> m_vector;

    size_t m_size;
    size_t m_mask;

    size_t m_readPtr;
    size_t m_writePtr;

    size_t m_bytesForRead;
    size_t m_bytesForWrite;

    mutex m_mutex;
    condition_variable m_condVarRead;
    condition_variable m_condVarWrite;
};

template <typename T>
RingBufferConcurrent<T>::RingBufferConcurrent(size_t t_size) :
  m_vector(pow2Next(t_size)),
  m_size(m_vector.size()),
  m_mask(std::max(0, static_cast<int32_t>(m_size) - 1)),
  m_readPtr(0),
  m_writePtr(0),
  m_bytesForRead(0),
  m_bytesForWrite(m_size)

{

}

template <typename T>
void RingBufferConcurrent<T>::resize(size_t t_size)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // проверка размера
    size_t t_newSize = pow2Next(t_size);
    if (t_newSize == m_vector.size())
        return;

    // изменяем размер вектора
    m_vector.resize(t_newSize);

    // инициализация полей
    m_size = m_vector.size();
    m_mask = std::max(0, static_cast<int32_t>(m_size) - 1);

    // сбрасываем счётчики
    m_bytesForRead  = 0;
    m_bytesForWrite = m_size;

    // обнуляем указатели чтения и записи
    m_readPtr  = 0;
    m_writePtr = 0;
}

template <typename T>
size_t RingBufferConcurrent<T>::size() const
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // возвращаем размер вектора
    return m_size;
}

template <typename T>
void RingBufferConcurrent<T>::clear()
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // сбрасываем счётчики это означает, что в нём нет данных
    // и обнулять каждый элемент вектора нет необходимости
    m_bytesForRead  = 0;
    m_bytesForWrite = m_size;

    // обнуляем указатели чтения и записи`
    m_readPtr  = 0;
    m_writePtr = 0;
}

template <typename T>
void RingBufferConcurrent<T>::write(const vector<T> &items)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // если длина вектора больше размера буфера, то игнорируем
    if (items.size() > m_size)
        return;

    // ожидаем возможности записи заданного числа элементов
    while (items.size() > m_bytesForWrite)
        m_condVarRead.wait(t_locker);

    // инициализируем счётчики
    m_bytesForWrite -= items.size();
    m_bytesForRead  += items.size();

    // запись данных в буфер
    for (auto &data : items)
        m_vector[m_writePtr++&m_mask] = data;

    // уведомляем, что появились данные для чтения
    m_condVarWrite.notify_one();
}

template <typename T>
void RingBufferConcurrent<T>::read(vector<T> &dst)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // если длина вектора больше размера буфера, то игнорируем
    if (dst.size() > m_size)
        return;

    // ожидаем возможности чтения из контейнера
    while (dst.size() > m_bytesForRead)
        m_condVarWrite.wait(t_locker);

    // инициализируем счётчики
    m_bytesForWrite += dst.size();
    m_bytesForRead  -= dst.size();

    // чтение данных из буфера
    for (T &data : dst)
        data = m_vector[m_readPtr++&m_mask];

    // уведомляем, что освободилось место для записи
    m_condVarRead.notify_one();
}

template <typename T>
bool RingBufferConcurrent<T>::tryWrite(const vector<T> &items, const chrono_ms duration)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // если длина вектора больше размера буфера, то игнорируем
    if (items.size() > m_size)
        return false;

    // функция - условие выхода из ожидания
    auto cmp = [&](){ return m_bytesForWrite >= items.size(); };

    // ожидаем возможности записи данных в течении заданного времени
    // если не дождались, то возвращаем false
    if (!m_condVarRead.wait_until(t_locker, sys_clock::now() + duration, cmp))
        return false;

    // инициализируем счётчики
    m_bytesForWrite -= items.size();
    m_bytesForRead  += items.size();

    // запись данных в буфер
    for (auto &data : items)
        m_vector[m_writePtr++&m_mask] = data;

    // уведомляем, что появились данные для чтения
    m_condVarWrite.notify_one();

    // данные были записаны в буфер, возвращаем true
    return true;
}

template <typename T>
bool RingBufferConcurrent<T>::tryRead(vector<T> &dst, const chrono_ms duration)
{
    // блокируем доступ
    unique_lock<mutex> t_locker(m_mutex);

    // если длина вектора больше размера буфера, то игнорируем
    if (dst.size() > m_size)
        return false;

    // функция - условие выхода из ожидания
    auto cmp = [&](){ return m_bytesForRead >= dst.size(); };

    // ожидаем возможности записи данных в течении заданного времени
    // если не дождались, то возвращаем false
    if (!m_condVarWrite.wait_until(t_locker, sys_clock::now() + duration, cmp))
        return false;

    // инициализируем счётчики
    m_bytesForWrite += dst.size();
    m_bytesForRead  -= dst.size();

    // чтение данных из буфера
    for (T &data : dst)
        data = m_vector[m_readPtr++&m_mask];

    // уведомляем, что освободилось место для записи
    m_condVarRead.notify_one();

    // данные были записаны в буфер, возвращаем true
    return true;
}

#endif // RINGBUFFERCONCURRENT_H

