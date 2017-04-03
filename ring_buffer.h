#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <QtCore>

using namespace std;
// the buffer size is always multiples of 2
template <typename T>
class RingBuffer
{
public:
    explicit RingBuffer(quint32 t_size = 0);

    void resize(quint32 t_size);
    quint32 size() const noexcept;

    void clear() noexcept;

    quint32 readCount() const noexcept;
    quint32 writeCount() const noexcept;

    bool read(QVector<T>  &dst) noexcept;
    bool write(QVector<T> &src) noexcept;

    bool read(T *pDst, quint32 len) noexcept;
    bool write(T *pSrc, quint32 len) noexcept;

private:
    RingBuffer(const RingBuffer &) = delete;
    RingBuffer &operator=(const RingBuffer &) = delete;

    inline int pow2Next(int d)
    {
        d--;
        d |= d >> 1;
        d |= d >> 2;
        d |= d >> 4;
        d |= d >> 8;
        d |= d >> 16;
        d++;
        return d;
    }

private:
    quint32 m_size;
    quint32 m_readPtr;
    quint32 m_writePtr;
    quint32 m_mask;

    atomic<quint32> m_bytesForRead;
    atomic<quint32> m_bytesForWrite;

    QVector<T> m_buffer;
    QMutex     m_mutex;
};

template <typename T>
RingBuffer<T>::RingBuffer(quint32 t_size) :
  m_size(t_size),
  m_readPtr(0),
  m_writePtr(0),
  m_mask(0),
  m_bytesForRead(0),
  m_bytesForWrite(0)
{
    m_buffer.resize(pow2Next(t_size));
    m_size = m_buffer.size();
    m_mask = m_size - 1;
    m_bytesForWrite = m_size;

}

template <typename T>
void RingBuffer<T>::resize(quint32 t_size)
{
    if (m_size == t_size)
        return;

    m_bytesForRead  = 0;
    m_bytesForWrite = 0;

    m_mutex.lock();
        m_buffer.resize(pow2Next(t_size));
    m_mutex.unlock();

    m_size = m_buffer.size();
    m_mask = m_size - 1;
    m_bytesForWrite = m_size;
}

template <typename T>
quint32 RingBuffer<T>::size() const noexcept
{
    return m_size;
}

template <typename T>
void RingBuffer<T>::clear() noexcept
{
    m_bytesForRead  = 0;
    m_bytesForWrite = 0;

    m_readPtr  = 0;
    m_writePtr = 0;

    m_bytesForWrite = m_size;
}

template <typename T>
quint32 RingBuffer<T>::readCount() const noexcept
{
    return m_bytesForRead;
}

template <typename T>
quint32 RingBuffer<T>::writeCount() const noexcept
{
    return m_bytesForWrite;
}

template <typename T>
bool RingBuffer<T>::read(QVector<T> & dst) noexcept
{
    if ((dst.size() > m_bytesForRead) || (dst.size() == 0))
        return false;

    m_bytesForRead -= dst.size();
    m_mutex.lock();
        for (T &data : dst)
            data = m_buffer[m_readPtr++&m_mask];
    m_mutex.unlock();
    m_bytesForWrite += dst.size();

    return true;
}

template <typename T>
bool RingBuffer<T>::write(QVector<T> & src) noexcept
{
    if ((src.size() > m_bytesForWrite) || (src.size() == 0))
        return false;

    m_bytesForWrite -= src.size();
    m_mutex.lock();
        for (T &data : src)
            m_buffer[m_writePtr++&m_mask] = data;
    m_mutex.unlock();
    m_bytesForRead += src.size();

    return true;
}

template <typename T>
bool RingBuffer<T>::read(T *pDst, quint32 len) noexcept
{
    if ((len > m_bytesForRead) || (len == 0u))
        return false;

    m_bytesForRead -= len;
    m_mutex.lock();
        for (quint32 i = 0; i < len; ++i)
            pDst[i] = m_buffer[m_readPtr++&m_mask];
    m_mutex.unlock();
    m_bytesForWrite += len;

    return true;
}

template <typename T>
bool RingBuffer<T>::write(T *pSrc, quint32 len) noexcept
{
    if ((len > m_bytesForWrite) || (len == 0u))
        return false;

    m_bytesForWrite -= len;
    m_mutex.lock();
        for (quint32 i = 0; i < len; ++i)
            m_buffer[m_writePtr++&m_mask] = pSrc[i];
    m_mutex.unlock();
    m_bytesForRead += len;

    return true;
}

#endif // RING_BUFFER_H
