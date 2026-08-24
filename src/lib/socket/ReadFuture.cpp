#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

//! Async Engine Includes
#include "ReadFuture.h"
#include "TCPClientSocket.h"
#include "RunTime.h"
#include "Events.h"
#include "Result.h"

AsyncIO::ReadFuture::ReadFuture(int fd, RunTime *p_pEventLoop, char *buffer, unsigned int size)
    : m_fd(fd), m_pEventLoop(p_pEventLoop), m_buffer(buffer), m_uisize(size)
{
}

AsyncIO::ReadFuture::~ReadFuture()
{
}

AsyncIO::ReadFuture *AsyncIO::ReadFuture::Spawn(int fd, RunTime *p_pRunTime, char *buffer, unsigned int size)
{
    auto fut = new ReadFuture(fd, p_pRunTime, buffer, size);
    fut->AttachToRunTime();
    return fut;
}

void AsyncIO::ReadFuture::AttachToRunTime()
{
    short eventsToSubTo = static_cast<short>(EventType::Read) | static_cast<short>(EventType::CLOSE);
    m_pEventLoop->RegisterFuture(m_fd, eventsToSubTo, this);
}

AsyncIO::FutureStatus AsyncIO::ReadFuture::Poll()
{
    int bytesRead = read(m_fd, m_buffer, m_uisize);
    if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        return FutureStatus::Pending;
    }
    if (bytesRead == -1)
    {
        m_iErrorCode = errno;
        return FutureStatus::Failed;
    }
    m_iBytesRead = bytesRead;
    return FutureStatus::Completed;
}

std::pair<char *, unsigned int> AsyncIO::ReadFuture::GetValue()
{
    return {m_buffer, static_cast<unsigned int>(m_iBytesRead)};
}
