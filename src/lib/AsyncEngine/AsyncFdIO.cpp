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
    m_pEventLoop->SubScribeToEvent(GetID(), AsyncIO::EventType::Read | AsyncIO::EventType::CLOSE, [this](AsyncIO::EventContext ctx)
                                   { HandleEventReady(ctx); });
}

void AsyncIO::AsyncFdIO::Write(const char *buffer, unsigned int size)
{
    int writtenBytes = write(GetID(), buffer, size);
    if (writtenBytes == -1)
    {
        throw std::runtime_error("[NOT-Implemented] Socket Write buffer is full, cannot write right now");
    }
    else
    {
        if (writtenBytes != size)
        {
            throw std::runtime_error("[FATAL]: not all bytes got written");
        }
    }
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
        throw std::runtime_error("[NOT-Implemented] Client socket space available");
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
    Close();
}