//! Async Engine Includes
#include "CallbackRegistry.h"
#include "PollableRegistery.h"

AsyncIO::CallbackSubscribeHandler::CallbackSubscribeHandler(AsyncIO::CallbackSubscribeHandler::CallbackSubscribeHandlerContext context)
    : m_oContext(std::move(context))
{
}

void AsyncIO::CallbackSubscribeHandler::HandleSubscriptionRequest(AsyncIO::PollableRegistery &registery)
{
    registery.m_oCallbackRegistry.Set(m_oContext.fd, std::move(m_oContext.callback));
    registery.m_oMonitoredFdRegistry.AddOrUpdate(m_oContext.fd, m_oContext.eventsToSubTo);
}

AsyncIO::CallbackUnSubscribeHandler::CallbackUnSubscribeHandler(AsyncIO::CallbackUnSubscribeHandler::CallbackUnSubscribeHandlerContext context)
    : m_oContext(std::move(context))
{
}

void AsyncIO::CallbackUnSubscribeHandler::HandleSubscriptionRequest(AsyncIO::PollableRegistery &registery)
{
    registery.m_oMonitoredFdRegistry.Remove(m_oContext.fd);
    registery.m_oCallbackRegistry.Remove(m_oContext.fd);
}

bool AsyncIO::CallbackRegistry::Contains(int fd) const
{
    return m_mapCallBacks.find(fd) != m_mapCallBacks.end();
}

void AsyncIO::CallbackRegistry::Set(int fd, std::function<void(EventContext)> callback)
{
    m_mapCallBacks[fd] = std::move(callback);
}

void AsyncIO::CallbackRegistry::Remove(int fd)
{
    m_mapCallBacks.erase(fd);
}
