//! System Includes
#include <unistd.h>

//! Async Engine
#include "AsyncFdIO.h"
#include "EventLoop.h"
#include "Events.h"
#include "PollUtils.h"

AsyncIO::AsyncFdIO::AsyncFdIO(EventLoop *p_pEventLoop) : m_pEventLoop(p_pEventLoop)
{
}

void AsyncIO::AsyncFdIO::SetFD(int fd)
{
    m_iFD = fd;
    AsyncIO::MakeNonBlocking(m_iFD);
    short eventsToSubTo = EventType::Read | EventType::CLOSE | EventType::WriteSpaceAvailable;
    m_pEventLoop->SubScribeToEvent(GetID(), eventsToSubTo, [this](AsyncIO::EventContext ctx)
                                   { HandleEventReady(ctx); });
}

bool AsyncIO::AsyncFdIO::WriteAll(const char *buffer, unsigned int size)
{
    //! Back pressure, caller should wait till space available
    //! (could be done with CV)
    if (m_pendingBuffer)
    {
        return false;
    }
    int uiWrittenBytes = write(GetID(), buffer, size);
    if (uiWrittenBytes == -1)
    {
        throw std::runtime_error("[FATAL] couldn't send data to send buffer (connection may have been interrupted)?");
    }
    else if (uiWrittenBytes == size)
    {
        //! Delete at once, since no buffering.
        delete[] buffer;
        ResetPendingBuffer();
    }
    else
    {
        m_pendingBuffer = buffer;
        m_uiPendingBufferSize = size;
        m_uiPendingBufferOffset = uiWrittenBytes;
    }
    return true;
}

void AsyncIO::AsyncFdIO::WriteFromPendingBuffer()
{
    unsigned int uiSizeToWrite = m_uiPendingBufferSize - m_uiPendingBufferOffset;
    int uiWrittenBytes = write(GetID(), m_pendingBuffer + m_uiPendingBufferOffset, uiSizeToWrite);

    if (uiWrittenBytes == uiSizeToWrite)
    {
        delete[] m_pendingBuffer;
        ResetPendingBuffer();
    }
    else
    {
        m_uiPendingBufferOffset += uiWrittenBytes;
    }
}

void AsyncIO::AsyncFdIO::ResetPendingBuffer()
{
    m_pendingBuffer = nullptr;
    m_uiPendingBufferOffset = 0;
    m_uiPendingBufferSize = 0;
}

void AsyncIO::AsyncFdIO::HandleEventReady(AsyncIO::EventContext ctx)
{
    if (AsyncIO::PollHelpers::IsEventSet(ctx.readyEvents, EventType::CLOSE))
    {
        HandleClose();
    }
    else if (AsyncIO::PollHelpers::IsEventSet(ctx.readyEvents, EventType::Read))
    {
        HandleDataReady();
    }
    else if (AsyncIO::PollHelpers::IsEventSet(ctx.readyEvents, EventType::WriteSpaceAvailable))
    {
        // if (m_pendingBuffer == nullptr)
        // {
        //     throw("[FATAL]: corrupted scenario, trying to send remaining buffer, when no pending data is available");
        // }
        if (m_pendingBuffer)
        {
            WriteFromPendingBuffer();
        }
    }
}

void AsyncIO::AsyncFdIO::HandleDataReady()
{
    unsigned int size = 1024;
    char buffer[size];
    int readBytes = read(GetID(), buffer, size);
    if (readBytes == -1)
    {
        throw std::runtime_error("[FATAL]: Error In Reading Client Bytes for socket FD " + std::to_string(GetID()));
    }
    else if (readBytes == 0)
    {
        throw std::runtime_error("[FATAL]: connection closed by socket.read which is unxpected, event loop handle it with higher priority than read!");
    }
    if (m_fOnReadHandler)
    {
        m_fOnReadHandler(buffer, readBytes);
    }
}

void AsyncIO::AsyncFdIO::HandleClose()
{
    Close();
    if (m_fOnCloseHandler)
    {
        m_fOnCloseHandler();
    }
}

void AsyncIO::AsyncFdIO::OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback)
{
    m_fOnReadHandler = std::move(p_fOnReadCallback);
}

void AsyncIO::AsyncFdIO::OnClose(std::function<void(void)> p_fOnCloseCallback)
{
    m_fOnCloseHandler = std::move(p_fOnCloseCallback);
}

int AsyncIO::AsyncFdIO::GetID()
{
    return m_iFD;
}

void AsyncIO::AsyncFdIO::Close()
{
    m_pEventLoop->UnRegisterFromAllEvents(GetID());
    close(GetID());
}

AsyncIO::AsyncFdIO::~AsyncFdIO()
{
    if (m_pendingBuffer)
    {
        delete[] m_pendingBuffer;
        ResetPendingBuffer();
    }
    Close();
}