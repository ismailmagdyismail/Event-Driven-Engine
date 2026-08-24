//! System Includes
#include <unistd.h>

//! Async Engine
#include "AsyncFdIO.h"
#include "RunTime.h"
#include "Events.h"
#include "PollUtils.h"
#include "ReadFuture.h"

AsyncIO::AsyncFdIO::AsyncFdIO(RunTime *p_pEventLoop) : m_pEventLoop(p_pEventLoop)
{
}

void AsyncIO::AsyncFdIO::SetFD(int fd, short flagsToSubTo)
{
    m_iFD = fd;
    AsyncIO::MakeNonBlocking(m_iFD);
    m_pEventLoop->SubScribeToEvent(GetID(), flagsToSubTo, [this](AsyncIO::EventContext ctx)
                                   { HandleEventReady(ctx); });
}

bool AsyncIO::AsyncFdIO::WriteAll(const char *buffer, unsigned int size, std::function<void(void)> p_fOnCompletion)
{
    //! TODO: differntiate between backpressure, and connection interrupted status code to know what to retry and what not
    //! Back pressure, caller should wait till space available
    //! (could be done with CV)
    if (m_pendingBuffer)
    {
        return false;
    }
    m_fOnWriteComplete = std::move(p_fOnCompletion);
    int iWrittenBytes = write(GetID(), buffer, size);
    if (ConnectionInterrupted(iWrittenBytes, errno))
    {
        ResetPendingBuffer();
        throw std::runtime_error("[FATAL] couldn't send data to send buffer (connection may have been interrupted)? " + std::to_string(errno));
    }
    else if (BackPressure(iWrittenBytes, errno))
    {
        m_pendingBuffer = buffer;
        m_uiPendingBufferSize = size;
        m_uiPendingBufferOffset = 0;
    }
    else if (iWrittenBytes == size)
    {
        p_fOnCompletion();
        ResetPendingBuffer();
    }
    else
    {
        m_pendingBuffer = buffer;
        m_uiPendingBufferSize = size;
        m_uiPendingBufferOffset = iWrittenBytes;
    }
    return true;
}

void AsyncIO::AsyncFdIO::WriteFromPendingBuffer()
{
    unsigned int uiSizeToWrite = m_uiPendingBufferSize - m_uiPendingBufferOffset;
    int iWrittenBytes = write(GetID(), m_pendingBuffer + m_uiPendingBufferOffset, uiSizeToWrite);
    if (ConnectionInterrupted(iWrittenBytes, errno))
    {
        ResetPendingBuffer();
        throw std::runtime_error("[FATAL] couldn't send data to send buffer (connection may have been interrupted)? " + std::to_string(errno));
        return;
    }
    else if (BackPressure(iWrittenBytes, errno))
    {
        return;
    }
    if (iWrittenBytes == uiSizeToWrite)
    {
        m_fOnWriteComplete();
        ResetPendingBuffer();
    }
    else
    {
        m_uiPendingBufferOffset += iWrittenBytes;
    }
}

bool AsyncIO::AsyncFdIO::ConnectionInterrupted(int iWrittenBytes, int errcode)
{
    return iWrittenBytes == -1 && errcode != EAGAIN;
}

bool AsyncIO::AsyncFdIO::BackPressure(int iWrittenBytes, int errcode)
{
    return iWrittenBytes == -1 && errcode == EAGAIN;
}

void AsyncIO::AsyncFdIO::ResetPendingBuffer()
{
    m_fOnWriteComplete = nullptr;
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
        HandleReadEvent();
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

void AsyncIO::AsyncFdIO::HandleReadEvent()
{
    if (m_fOnDataAvailableHandler)
    {
        m_fOnDataAvailableHandler();
    }
    else
    {
        HandleReadData();
    }
}

void AsyncIO::AsyncFdIO::HandleReadData()
{
    unsigned int size = 1024;
    char buffer[size];
    int readBytes = ReadSync(buffer, size);
    if (m_fOnReadHandler)
    {
        m_fOnReadHandler(buffer, readBytes);
    }
}

AsyncIO::ReadFuture *AsyncIO::AsyncFdIO::Read(char *buffer, unsigned int size)
{
    return ReadFuture::Spawn(m_iFD, m_pEventLoop, buffer, size);
}

int AsyncIO::AsyncFdIO::ReadSync(char *buffer, unsigned int size)
{
    int readBytes = read(GetID(), buffer, size);
    if (readBytes == -1)
    {
        throw std::runtime_error("[FATAL]: Error In Reading Client Bytes for socket FD " + std::to_string(GetID()));
    }
    else if (readBytes == 0)
    {
        throw std::runtime_error("[FATAL]: connection closed by socket.read which is unxpected, event loop handle it with higher priority than read!");
    }
    return readBytes;
}

void AsyncIO::AsyncFdIO::HandleClose()
{
    Close();
    if (m_fOnCloseHandler)
    {
        m_fOnCloseHandler();
    }
}

void AsyncIO::AsyncFdIO::OnDataAvailable(std::function<void(void)> p_fonDataAvailableCallback)
{
    m_fOnDataAvailableHandler = std::move(p_fonDataAvailableCallback);
    m_fOnReadHandler = nullptr;
}

void AsyncIO::AsyncFdIO::OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback)
{
    m_fOnReadHandler = std::move(p_fOnReadCallback);
    m_fOnDataAvailableHandler = nullptr;
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
    if (m_iFD == -1)
    {
        return;
    }
    m_pEventLoop->UnRegisterFromAllEvents(GetID());
    close(GetID());
    m_iFD = -1;
}

AsyncIO::AsyncFdIO::~AsyncFdIO()
{
    Close();
}