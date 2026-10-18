// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./console.h"

#if defined(Q_OS_UNIX)
#  include <errno.h>
#  include <termios.h>
#  include <unistd.h>
#elif defined(Q_OS_WIN)
#  include <conio.h>
#endif

/*!
    @class Console
    @internal

    Asks the person at a tool's terminal (or an agent that drives the
    tool): it writes each question to the output device, and reads the
    answer from its input device.
*/

/*!
    Builds the console that reads the answers from @a input, and writes
    the questions to @a output.
*/
Console::Console(QIODevice *input, QIODevice *output)
    : m_input(input), m_output(output), m_terminalFd(-1), m_styled(false)
{
}

/*!
    Asks @a message as a yes or no question, the way build-handler.sh's
    `bh_yes_no` does, and returns false only for a no (`n` or `no`, in any
    case); an empty answer, and an input that ended, count as yes.
*/
bool Console::confirm(const QString & message)
{
    const QString answer = this->prompt(message + QLL(" [Y/n]")).trimmed().toLower();
    return answer != QLL("n") && answer != QLL("no");
}

/*!
    Tells the console that its input is the terminal of the file descriptor
    @a inputFd (-1 for none), for page() to read it key by key, and whether
    its output shows colors (@a styled).
*/
void Console::setTerminal(int inputFd, bool styled)
{
    this->m_terminalFd = inputFd;
    this->m_styled = styled;
}

/*!
    Shows @a text the way a git-style pager does, then waits at its bottom
    line, `: (type q to continue)`, until the reader answers `q` (in any
    case); an input that ended counts as `q` as well.

    On a terminal (see setTerminal()), the line works the way the pager's
    does: the `:` shows white and the hint gray (with styled output), the
    hint gives way to what the reader types and comes back once the line is
    empty again, a `q` on an empty line continues at once, and Enter submits
    what was typed (which continues for `q`, and clears the line otherwise).
    Elsewhere (an agent's input, such as a FIFO), the line shows as plain
    text, and each answered line that is not `q` asks again.
*/
void Console::page(const QString & text)
{
    this->m_output->write(text.toLocal8Bit());
    if ( ! text.isEmpty() && ! text.endsWith(QLatin1Char('\n'))) {
        this->m_output->write("\n");
    }
    if (this->m_terminalFd >= 0 && this->pageOnTerminal()) {
        return;
    }
    for (;;) {
        this->m_output->write(": (type q to continue) ");
        bool atEnd = false;
        const QString answer = this->readLine(&atEnd).trimmed().toLower();
        if (atEnd || answer == QLL("q")) {
            return;
        }
    }
}

/*!
    Draws the pager's bottom line again: the `:` and, while nothing is typed,
    the gray hint, else @a typed.
*/
void Console::drawPagerLine(const QString & typed)
{
    QByteArray line = "\r\033[K";
    line += this->m_styled
        ? QByteArray("\033[97m:\033[0m ") : QByteArray(": ");
    if (typed.isEmpty()) {
        line += this->m_styled ? QByteArray("\033[90m(type q to continue)\033[0m\r\033[2C")
                               : QByteArray("(type q to continue)\r\033[2C");
    } else {
        line += typed.toLocal8Bit();
    }
    this->m_output->write(line);
}

/*!
    Waits at the pager's bottom line on the terminal, key by key (see
    page()); returns false when the terminal takes no key-by-key input,
    hence the caller falls back to whole lines.
*/
bool Console::pageOnTerminal()
{
#if defined(Q_OS_UNIX)
    // Qt has no interface for key-by-key terminal input; hence the
    // platform's own termios calls.
    termios saved;
    if (tcgetattr(this->m_terminalFd, &saved) != 0) {
        return false;
    }
    termios raw = saved;
    raw.c_lflag &= ~tcflag_t(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(this->m_terminalFd, TCSANOW, &raw) != 0) {
        return false;
    }
#elif !defined(Q_OS_WIN)
    return false;
#endif
    QString typed;
    this->drawPagerLine(typed);
    for (;;) {
        // Reads one key.
        int key = -1;
#if defined(Q_OS_UNIX)
        char c = 0;
        ssize_t got = -1;
        do {
            got = ::read(this->m_terminalFd, &c, 1);
        } while (got < 0 && errno == EINTR);
        key = got == 1 ? uchar(c) : -1;
#elif defined(Q_OS_WIN)
        // Qt has no interface for key-by-key console input; hence the C
        // runtime's own `_getch()`.
        key = _getch();
#endif
        if (key < 0 || key == 4) {
            break;
        }

        // Continues on a `q`, and edits the line otherwise.
        if ((key == 'q' || key == 'Q') && typed.isEmpty()) {
            break;
        }
        if (key == '\r' || key == '\n') {
            if (typed.trimmed().toLower() == QLL("q")) {
                break;
            }
            typed.clear();
        } else if (key == 127 || key == 8) {
            typed.chop(1);
        } else if (key >= 32) {
            typed += QChar(key);
        }
        this->drawPagerLine(typed);
    }
    this->m_output->write("\r\033[K");
#if defined(Q_OS_UNIX)
    tcsetattr(this->m_terminalFd, TCSANOW, &saved);
#endif
    return true;
}

/*!
    Asks @a message, and returns the line answered, without its newline.
*/
QString Console::prompt(const QString & message)
{
    this->m_output->write(message.toLocal8Bit() + ' ');
    bool atEnd = false;
    return this->readLine(&atEnd);
}

/*!
    Asks @a message, and returns the answer of one or more lines: a line
    that ends with a backslash continues on the next one (the backslash
    taken out, the newline kept), and a line without one ends the answer.
*/
QString Console::promptMulti(const QString & message)
{
    this->m_output->write(message.toLocal8Bit() + ' ');
    QString answer;
    for (;;) {
        bool atEnd = false;
        QString line = this->readLine(&atEnd);
        if ( ! line.endsWith(QLatin1Char('\\')) || atEnd) {
            answer += line;
            return answer;
        }
        line.chop(1);
        answer += line + QLatin1Char('\n');
    }
}

/*!
    Reads one line from the input, without its newline (and carriage
    return), and sets @a atEnd once the input has ended.
*/
QString Console::readLine(bool *atEnd)
{
    QByteArray line;
    for (;;) {
        char c = 0;
        if ( ! this->m_input->getChar(&c)) {
            if ( ! this->m_input->waitForReadyRead(-1)
                || ! this->m_input->getChar(&c)
            ) {
                *atEnd = true;
                break;
            }
        }
        if (c == '\n') {
            break;
        }
        line += c;
    }
    if (line.endsWith('\r')) {
        line.chop(1);
    }
    return QString::fromLocal8Bit(line);
}
