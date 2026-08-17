#include "TerminalIO.h"

AsyncIO::TerminalIO::TerminalIO(AsyncIO::TerminalIO::TerminalType p_eType, AsyncIO::EventLoop *p_pLoop)
    : m_eType(p_eType), m_oAsyncFDIO(p_pLoop)
{
    m_oAsyncFDIO.SetFD(static_cast<int>(m_eType));
}

void AsyncIO::TerminalIO::WriteAll(char *buffer, unsigned int size, std::function<void(void)> p_fOnCompletion)
{
    m_oAsyncFDIO.WriteAll(buffer, size, std::move(p_fOnCompletion));
}

void AsyncIO::TerminalIO::OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback)
{
    m_oAsyncFDIO.OnRead(p_fOnReadCallback);
}

void AsyncIO::TerminalIO::OnClose(std::function<void(void)> p_fOnCloseCallback)
{
    m_oAsyncFDIO.OnClose(std::move(p_fOnCloseCallback));
}

int AsyncIO::TerminalIO::GetID()
{
    return m_oAsyncFDIO.GetID();
}

void AsyncIO::TerminalIO::Close()
{
    m_oAsyncFDIO.Close();
}