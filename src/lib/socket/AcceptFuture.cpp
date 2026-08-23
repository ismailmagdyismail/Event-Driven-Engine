#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

//! Async Engine Includes
#include "AcceptFuture.h"
#include "TCPClientSocket.h"
#include "RunTime.h"
#include "Events.h"
#include "Result.h"

AsyncIO::AcceptFuture::AcceptFuture(int fd, RunTime *p_pEventLoop) : m_fd(fd), m_pEventLoop(p_pEventLoop)
{
    AttachToRunTime();
}

AsyncIO::AcceptFuture *AsyncIO::AcceptFuture::Spawn(int fd, RunTime *p_pRunTime)
{
    AcceptFuture *pFuture = new AcceptFuture(fd, p_pRunTime);
    return pFuture;
}

AsyncIO::AcceptFuture::~AcceptFuture()
{
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

void AsyncIO::AcceptFuture::Then(std::function<void(std::unique_ptr<AsyncIO::TCPClientSocket>)> callback)
{
    m_fContinuationCallback = [this, cb = std::move(callback)]()
    {
        AsyncIO::SocketInfo clientSockInfo{.fd = m_iCreatedClientSocketFD};
        cb(std::make_unique<AsyncIO::TCPClientSocket>(m_pEventLoop, clientSockInfo));
    };
}

void AsyncIO::AcceptFuture::Catch(std::function<void(Result)> callback)
{
    // m_fOnErrorCallback = std::move(callback);
    // Result result{.success = false, .message = "Failed to accept connection with error code: " + std::to_string(m_iErrorCode)};
    // m_fOnErrorCallback(result);
}