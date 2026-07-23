#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

template <typename T>
class MPSCQueue
{
public:
    MPSCQueue(unsigned int maxSize)
    {
        m_uiMaxSize = maxSize;
        m_bIsReading.store(true, std::memory_order_relaxed);
        m_bIsWriting.store(true, std::memory_order_relaxed);
        m_uiSize.store(0, std::memory_order_relaxed);
        m_uiReadPosition = 0;
        m_uiWritePosition = 0;
        m_vecCircularBuffer.resize(maxSize, T{});
    }

    bool Push(T object)
    {
        bool bIsWriting = true;
        while ((bIsWriting = m_bIsWriting.load(std::memory_order_relaxed)))
        {
            //! Read the most up to date size / snapshot
            //! TRY updating it by reserving
            unsigned int uiSizeSnapshot = m_uiSize.load(std::memory_order_relaxed);

            //! 1. Has no space, we yield till consumer pop off
            if (uiSizeSnapshot >= m_uiMaxSize)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            //! 2. Has space
            //! 2.1 another producer(s) may have beaten us to the slot we expect to reserve
            //! 2.2 no producers beat use we can insert safely
            unsigned int uiSizeToUpdate = uiSizeSnapshot + 1;
            if (!m_uiSize.compare_exchange_strong(uiSizeSnapshot, uiSizeToUpdate, std::memory_order_release))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            //! 2.2 if succeeded to break out of the loop, it means we owns the index, can write to it
            m_vecCircularBuffer[m_uiWritePosition] = object;
            UpdateIndex(m_uiWritePosition);
            return true;
        }
        return bIsWriting;
    }

    bool Pop(T &readObject)
    {
        //! Since only one reader, we can safely load readPosition and cache it early on and avoid re-loading it again
        bool bIsReading = true;
        while (m_uiSize.load(std::memory_order_acquire) == 0 && (bIsReading = m_bIsReading.load(std::memory_order_relaxed)))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (!bIsReading)
        {
            return false;
        }

        readObject = std::move(m_vecCircularBuffer[m_uiReadPosition]);
        UpdateIndex(m_uiReadPosition);

        //! No need for a two way barrier here, since writers don't acces Written Elements / refrence them
        //! They just place them in the queue without processing
        m_uiSize.fetch_sub(1, std::memory_order_relaxed);

        return true;
    }

    void StopReading()
    {
        m_bIsReading.store(false, std::memory_order_relaxed);
    }

    void StopWriting()
    {
        m_bIsWriting.store(false, std::memory_order_relaxed);
    }

private:
    void UpdateIndex(unsigned int &p_iPosition)
    {
        unsigned int uiNextPosition = p_iPosition + 1;
        if (uiNextPosition >= m_uiMaxSize)
        {
            p_iPosition = 0;
        }
        else
        {
            ++p_iPosition;
        }
    }

    static const unsigned int CACHE_LINE_SIZE = 64;

    alignas(MPSCQueue::CACHE_LINE_SIZE) unsigned int m_uiMaxSize;
    alignas(MPSCQueue::CACHE_LINE_SIZE) unsigned int m_uiReadPosition;
    alignas(MPSCQueue::CACHE_LINE_SIZE) unsigned int m_uiWritePosition;
    alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<unsigned int> m_uiSize;
    alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<bool> m_bIsReading;
    alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<bool> m_bIsWriting;

    std::vector<T> m_vecCircularBuffer;
};