#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

//! Async Engine Includes
#include "AcceptFuture.h"
#include "TCPClientSocket.h"
#include "RunTime.h"
#include "Events.h"
#include "Result.h"

AsyncIO::AcceptFuture::AcceptFuture(int fd, RunTime *p_pEventLoop)
    : m_pEventLoop(p_pEventLoop), m_fd(fd)
{
}

AsyncIO::AcceptFuture::~AcceptFuture()
{
}

AsyncIO::AcceptFuture *AsyncIO::AcceptFuture::Spawn(int fd, RunTime *p_pRunTime)
{
    auto fut = new AcceptFuture(fd, p_pRunTime);
    fut->AttachToRunTime();
    std::cerr << "registerd to runtime" << std::endl;
    return fut;
}

void AsyncIO::AcceptFuture::AttachToRunTime()
{
    short eventsToSubTo = EventType::Read | EventType::CLOSE;
    m_pEventLoop->RegisterFuture(m_fd, eventsToSubTo, this);
}

AsyncIO::FutureStatus AsyncIO::AcceptFuture::Poll()
{
    int status = accept(m_fd, nullptr, nullptr);
    if (status == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        return FutureStatus::Pending;
    }
    else if (status == -1)
    {
        m_iErrorCode = errno;
        return FutureStatus::Failed;
    }
    m_iCreatedClientSocketFD = status;
    return FutureStatus::Completed;
}

std::unique_ptr<AsyncIO::TCPClientSocket> AsyncIO::AcceptFuture::GetValue()
{
    AsyncIO::SocketInfo clientSockInfo{.fd = m_iCreatedClientSocketFD};
    return std::make_unique<AsyncIO::TCPClientSocket>(m_pEventLoop, clientSockInfo);
}