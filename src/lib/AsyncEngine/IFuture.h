#pragma once

#include <functional>

namespace AsyncIO
{
    enum class FutureStatus
    {
        Pending,
        Completed,
        Failed
    };

    class IFuture
    {
    public:
        struct Context
        {
            int id;
            short event;
        };

        virtual ~IFuture() = default;
        virtual FutureStatus Poll() = 0;
        std::function<void(void)> GetContinuation()
        {
            return m_fContinuationCallback;
        }

    protected:
        std::function<void(void)> m_fContinuationCallback{nullptr};
    };

}