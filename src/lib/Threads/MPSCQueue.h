#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

template <typename T>
class MPSCQueue
{
public:
    static const unsigned int CACHE_LINE_SIZE = 64;

    struct Slot
    {
        alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<unsigned int> m_uiVersion;
        T TObject;
    };

    MPSCQueue(unsigned int maxSize)
    {
        m_uiMaxSize = maxSize;
        m_bIsReading.store(true, std::memory_order_relaxed);
        m_bIsWriting.store(true, std::memory_order_relaxed);
        m_uiHead = 0;
        m_uiTail.store(0, std::memory_order_relaxed);
        m_vecCircularBuffer.reserve(maxSize);
        for (unsigned int i = 0; i < maxSize; ++i)
        {
            auto pSlot = std::make_unique<Slot>();
            pSlot->m_uiVersion.store(i, std::memory_order_relaxed);
            pSlot->TObject = T{};
            m_vecCircularBuffer.push_back(std::move(pSlot));
        }
    }

    bool Push(T object)
    {
        if (!m_bIsWriting.load(std::memory_order_relaxed))
        {
            return false;
        }

        //! 1. Race to Reserve a Logical slot
        unsigned int uiLogicalPosition = m_uiTail.fetch_add(1, std::memory_order_relaxed);

        //! 2. Map it to physical location
        unsigned int uiPhysicalPosition = GetPositionWithinBounds(uiLogicalPosition);

        bool bReading = true;
        unsigned int uiVersion = 0;
        //! 3. wait till that physical location is actually supposed to be written to by that logical slot
        //! in other words wait till physical == logical
        //! this is to avoid overwriting data that are yet to be consumed
        //! when consumed logical version counter is incremented so it increases till expected logical producer counter
        do
        {
            bReading = m_bIsReading.load(std::memory_order_relaxed);
            uiVersion = m_vecCircularBuffer[uiPhysicalPosition]->m_uiVersion.load(std::memory_order_acquire);
        } while (bReading && uiVersion < uiLogicalPosition);

        if (!bReading)
        {
            return false;
        }

        // 4. Insert into circular buffer
        m_vecCircularBuffer[uiPhysicalPosition]->TObject = std::move(object);

        // 5. publish to be seen by consumer
        m_vecCircularBuffer[uiPhysicalPosition]->m_uiVersion.store(uiLogicalPosition + 1, std::memory_order_release);

        return true;
    }

    bool Pop(T &readObject)
    {
        bool bReading = true;
        unsigned int uiPhysicalReadLocation = GetPositionWithinBounds(m_uiHead);
        bool bHasElementToRead = true;
        do
        {
            bReading = m_bIsReading.load(std::memory_order_relaxed);
            unsigned int uiPublishedVersion = m_vecCircularBuffer[uiPhysicalReadLocation]->m_uiVersion.load(std::memory_order_acquire);
            bHasElementToRead = (uiPublishedVersion == m_uiHead + 1);
        } while (!bHasElementToRead && bReading);

        if (!bReading)
        {
            return false;
        }
        readObject = std::move(m_vecCircularBuffer[uiPhysicalReadLocation]->TObject);
        m_vecCircularBuffer[uiPhysicalReadLocation]->m_uiVersion.store(m_uiHead + m_uiMaxSize, std::memory_order_release);
        ++m_uiHead;
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
    unsigned int GetPositionWithinBounds(unsigned int idx)
    {
        return idx % m_uiMaxSize;
    }

    //! Prevent false sharing
    alignas(MPSCQueue::CACHE_LINE_SIZE) unsigned int m_uiMaxSize;
    alignas(MPSCQueue::CACHE_LINE_SIZE) unsigned int m_uiHead;
    alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<unsigned int> m_uiTail;
    alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<bool> m_bIsReading;
    alignas(MPSCQueue::CACHE_LINE_SIZE) std::atomic<bool> m_bIsWriting;

    std::vector<std::unique_ptr<MPSCQueue::Slot>> m_vecCircularBuffer;
};