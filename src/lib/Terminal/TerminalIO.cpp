#include "TerminalIO.h"
#include "Events.h"

#include <unistd.h>

AsyncIO::TerminalIO::TerminalIO(RunTime *p_pLoop)
    : m_oStdIn(p_pLoop),
      m_oStdOut(p_pLoop)
{
    m_oStdIn.SetFD(STDIN_FILENO, EventType::Read);
    m_oStdOut.SetFD(STDOUT_FILENO, EventType::CLOSE | EventType::WriteSpaceAvailable);
}

bool AsyncIO::TerminalIO::WriteAll(char *buffer, unsigned int size, std::function<void(void)> p_fOnCompletion)
{
    return m_oStdOut.WriteAll(buffer, size, std::move(p_fOnCompletion));
}

void AsyncIO::TerminalIO::OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback)
{
    m_oStdIn.OnRead(std::move(p_fOnReadCallback));
}

void AsyncIO::TerminalIO::OnDataAvailable(std::function<void(TerminalIO &)> p_fOnDataAvailableCallback)
{
    m_oStdIn.OnDataAvailable([this, cb = std::move(p_fOnDataAvailableCallback)]()
                             { cb(*this); });
}

int AsyncIO::TerminalIO::ReadSync(char *buffer, unsigned int size)
{
    return m_oStdIn.ReadSync(buffer, size);
}

void AsyncIO::TerminalIO::OnClose(std::function<void(void)> p_fOnCloseCallback)
{
    // Terminal close events are generally associated with stdin.
    m_oStdIn.OnClose(std::move(p_fOnCloseCallback));
}

void AsyncIO::TerminalIO::Close()
{
    m_oStdIn.Close();
    m_oStdOut.Close();
}