#pragma once

#include <functional>
#include "Result.h"
#include <iostream>

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
        virtual ~IFuture() = default;
        virtual FutureStatus Poll() = 0;
        virtual std::function<void()> GetContinuation() = 0;
        virtual std::function<void(Result)> GetErrorContinuation() = 0;
    };

    // ============================================================================
    // FutureHandle<T> - Bridges the gap between futures in a chain
    //
    // KEY INSIGHT: The handle itself is NOT polled by the runtime.
    // Instead, when Bind() is called, it immediately sets up the next future
    // and registers it with the runtime.
    // ============================================================================
    template <typename T>
    class FutureHandle
    {
    public:
        using FutureType = T;
        using ValueType = typename T::ValueType;

        ~FutureHandle()
        {
            // If never bound, we leak nothing - the previous future's callback
            // owns the creation of the next future
        }

        void Then(std::function<void(ValueType)> callback)
        {
            if (m_pBoundFuture)
                m_pBoundFuture->Then(std::move(callback));
            else
                m_fPendingThen = std::move(callback);
        }

        void Catch(std::function<void(Result)> callback)
        {
            if (m_pBoundFuture)
                m_pBoundFuture->Catch(std::move(callback));
            else
                m_fPendingCatch = std::move(callback);
        }

        void Bind(T *future)
        {
            m_pBoundFuture = future;

            // Attach pending callbacks BEFORE registering with runtime
            if (m_fPendingThen)
                m_pBoundFuture->Then(std::move(m_fPendingThen));
            if (m_fPendingCatch)
                m_pBoundFuture->Catch(std::move(m_fPendingCatch));

            // NOW register with runtime - all callbacks are in place
            // m_pBoundFuture->AttachToRunTime();
        }

    private:
        T *m_pBoundFuture{nullptr};
        std::function<void(ValueType)> m_fPendingThen;
        std::function<void(Result)> m_fPendingCatch;
    };

    // ------------------------------------------------------------------
    // BaseFuture<T> - Templated base for all futures
    // T = the value type passed to Then() callback
    // ------------------------------------------------------------------
    template <typename T>
    class BaseFuture : public IFuture
    {
    public:
        using ValueType = T;

        void Then(std::function<void(T)> callback)
        {
            m_fValueCallback = std::move(callback);
            m_fContinuationCallback = [this]()
            {
                if (m_fValueCallback)
                    m_fValueCallback(GetValue());
            };
        }

        template <typename Func>
        auto Then(Func &&callback) -> std::enable_if_t<
            std::is_base_of_v<IFuture, std::remove_pointer_t<std::invoke_result_t<Func, T>>>,
            std::unique_ptr<FutureHandle<std::remove_pointer_t<std::invoke_result_t<Func, T>>>>>
        {
            using NextFutureType = std::remove_pointer_t<std::invoke_result_t<Func, T>>;
            auto handle = std::make_unique<FutureHandle<NextFutureType>>();

            m_fContinuationCallback = [this, cb = std::forward<Func>(callback), h = handle.get()]()
            {
                NextFutureType *nextFuture = cb(GetValue());
                h->Bind(nextFuture);
            };

            return handle;
        }

        void Catch(std::function<void(Result)> callback)
        {
            m_fErrorContinuation = std::move(callback);
        }

        std::function<void()> GetContinuation() override { return m_fContinuationCallback; }
        std::function<void(Result)> GetErrorContinuation() override { return m_fErrorContinuation; }

    protected:
        virtual T GetValue() = 0;

    private:
        std::function<void(T)> m_fValueCallback;
        std::function<void()> m_fContinuationCallback;
        std::function<void(Result)> m_fErrorContinuation;
    };

}