// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

// CASE	args-wrap-aligned-chain.cpp
void run(Document *document, int open, int close)
{
    const QByteArray text = sourceText(
        document,
        open + 1,
        close - 1
    );
}

// CASE	args-wrap-all-items-all.cpp
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(
                line.indexOf(
                    ' ',
                    line.indexOf(
                        ' ',
                        line.indexOf('x'),
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1
                    ) + 1
                ) + 1
            );
        }
    }
}

// CASE	args-wrap-all-items-condition.cpp
void update()
{
    {
        {
            {
                {
                    c->m_linkLocalIPv4 = (val.compare(
                        QLL("true"),
                        Qt::CaseInsensitive
                    ) == 0
                        || val.toInt() != 0);
                }
            }
        }
    }
}

// CASE	args-wrap-all-items.cpp
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(
                line.indexOf(
                    ' ',
                    line.indexOf(
                        ' ',
                        line.indexOf('x'),
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1
                    ) + 1
                ) + 1
            );
        }
    }
}

// CASE	args-wrap-chain.cpp
QByteArray lastField(const QByteArray & record)
{
    if (record.isEmpty()) {
        return QByteArray();
    }
    {
        {
            const QByteArray field = record.mid(
                record.indexOf(
                    ':',
                    record.indexOf(
                        ':',
                        record.indexOf(':', record.indexOf(':') + 1) + 1
                    ) + 1
                ) + 1
            );
            return field;
        }
    }
}

// CASE	args-wrap-logical-priority.cpp
bool matches()
{
    return acceptsToken(next)
        || ((next == DOT || next == ARROW)
            && hasJoinedExpression(document, before))
        || document->token(before) == EQ;
}

bool alternate()
{
    return cachedResult
        || findAlternateRecord(firstCandidate, secondCandidate);
}

bool validate()
{
    return isAvailable && validateExtraordinarilyLongArguments(
        firstExcessivelyLongCandidateName,
        secondExcessivelyLongCandidateName
    );
}

// CASE	args-wrap-matched-closings.cpp
void certificate()
{
    build(
        QLL("subjectAltName=DNS:") + QtNetworkSettings::serverLocalName()
            + QLatin1Char('.') + QtNetworkSettings::serverDomainName()
    );
    build(
        QLL("subjectAltName=DNS:*.wildcard.dev.")
            + QtNetworkSettings::serverDomainName(),
        &wildcardCa()
    );
    build(
        values[index]
            + remainder
    );
    const Values values = Values{
        Value{first}
            + remainder
    };
}

// CASE	args-wrap-operator-lines.cpp
void sum(Log *log, const QList<int > & values, int count, int total)
{
    log->line(
        QString::number(count).rightJustified(8, QLatin1Char(' '))
            .toUpper()
            .trimmed(),
        values[1] + values[2]
            + count
            + total
    );
}

// CASE	args-wrap-range-end.cpp
void report(Log *log, const QString & suffix, const QList<int > & values, int index)
{
    log->line(
        QString::fromLatin1("the first part of a rather long message to the user and ")
            + suffix,
        values[index + 1] + values[index + 2]
            + values[index + 3]
            + values[index + 4]
            + values[index + 5],
        u8"another long piece of text that keeps on going for quite a while longer"
        + suffix,
        "a plain literal piece of text that is also long enough to wrap right here"
        + suffix
    );
}

// CASE	args-wrap-single-broken.cpp
QByteArray lastField(const QByteArray & record)
{
    {
        {
            const QByteArray field = record.mid(
                record.indexOf(
                    ':',
                    record.indexOf(
                        ':',
                        record.indexOf(':', record.indexOf(':') + 1)
                            + 1
                    ) + 1
                ) + 1
            );
            return field;
        }
    }
}

// CASE	args-wrap-string-end.cpp
void report(Log *log, const QString & prefix, const QString & suffix, int count)
{
    log->line(
        QString::number(count).rightJustified(8, QLatin1Char(' '))
            .toUpper()
            .trimmed(),
        prefix + "a literal piece of text that is long enough to wrap"
        + suffix
    );
}

// CASE	args-wrap-string-lines.cpp
void writeAll()
{
    const QString file = this->writeFile(
        QString(QLL("rewrite/a.cpp")),
        "void f()\n{\n    // The tool waits for the server to answer\n"
            "    // each request in turn.\n    g(first,\n      second);\n}\n"
    );
}

// CASE	args-wrap-trailing-comma.cpp
int rankOf(Token token)
{
    static const Token ranks[][4] = {
        { OR, OR, OR, OR },
        { STAR, SLASH, PERCENT, PERCENT },
    };
    for (int r = 0; r < 2; ++r) {
        if (ranks[r][0] == token) {
            return r;
        }
    }
    return -1;
}

// CASE	args-wrap-whole-last-over.cpp
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(
                line.indexOf(
                    ' ',
                    ' ',
                    line.indexOf(' ', line.indexOf(' ') + 1) + 1,
                    line.indexOf(' ', line.indexOf(' ') + 1) + 1
                ) + 1
            );
        }
    }
}

// CASE	args-wrap-whole-last-two.cpp
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(
                line.indexOf(
                    ' ',
                    line.indexOf(
                        ' ',
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1,
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1
                    ) + 1
                ) + 1
            );
        }
    }
}

// CASE	args-wrap-whole-last.cpp
QByteArray lastWord(const QByteArray & text)
{
    {
        {
            const QByteArray word = text.mid(
                text.indexOf(
                    ':',
                    text.indexOf(
                        ':',
                        text.indexOf(':', text.indexOf(':') + 1) + 1
                    ) + 1
                ) + 1
            );
            return word;
        }
    }
}

// CASE	body-braces.h
class DummyEngine
{
public:
    int count() const { return m_count; }
    int total() const { return count(); }

private slots:
    void doSend(const QByteArray &verb, const QByteArray &id)
    {
        if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
            m_sock->write(verb + ' ' + id + '\n');
            m_sock->flush();
        }
    }

    void flush()
    { m_sock->write(m_buffer);
        m_sock->flush(); }

    void reset()
    {
        if (m_sock) {
            m_sock->abort();
        }
    }

    void send(const QByteArray &data)
    {
        m_sock->write(QByteArray(data.constData(),
            data.size())
        );
    }

private:
    QTcpSocket *m_sock;
    int m_count;
};

namespace Fake {
void touch() { if (m_count) { ++m_count; }
}
}

// CASE	condition-wrap-compare-all.cpp
bool isActive(const Item *item)
{
    for (int i = 0; i < 1; ++i) {
        {
            if ((item->value()->state & ItemConfiguration::Active)
                    == ItemConfiguration::Active
            ) {
                return true;
            }
        }
    }
    return false;
}

// CASE	condition-wrap-compare-fit.cpp
void pick(bool ready)
{
    if (ready
        && typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::Active
    ) {
        use();
    }
    if (ready
        && typeNameIsVeryVeryLongInThisCase
            == QNetworkConfiguration::UserChoiceValueThatIsLonger
    ) {
        use();
    }
}

// CASE	condition-wrap-compare-max.cpp
int kindOf(const Config & publicConfig)
{
    if (publicConfig.isValid()) {
        return 0;
    } else if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        return 1;
    }
    if (publicConfig.isValid()
        && publicConfig.type() == Network::Service
    ) {
        return 2;
    }
    return 3;
}

// CASE	condition-wrap-compare.cpp
bool isActive(const Item *item)
{
    for (int i = 0; i < 1; ++i) {
        {
            if ((item->value()->state & ItemConfiguration::Active)
                    == ItemConfiguration::Active
            ) {
                return true;
            }
        }
    }
    return false;
}

// CASE	condition-wrap-nested.cpp
bool pick(int token, int k, const Document *document)
{
    {
        {
            if ((token == LPAREN
                    || token == LBRACK)
                && document->match(k) > k
            ) {
                return true;
            }
            if ((first == second
                    || third == fourth
                    || fifth == sixth
                    || seventh == eighth)
                && ready
            ) {
                return false;
            }
        }
    }
    return false;
}

// CASE	condition-wrap-split.cpp
void poll(bool hasData, bool isBuffered, int socketType, Engine *engine)
{
    if (!hasData
        && !isBuffered
        && socketType == UdpSocketType
        && engine->isOpen()
        && engine->lastError() == ConnectionRefusedError
    ) {
        engine->reset();
    }
}

// CASE	continuation-indent-branches.cpp
bool keep(const QString & key)
{
    const bool useVariable = key == QLL("PATH") || key == QLL("QT_QPA_PLATFORM")
#if defined(Q_OS_UNIX)
        || key == QLL("HOME") || key == QLL("USER")
#  if defined(Q_OS_MAC)
        || key == QLL("DYLD_LIBRARY_PATH")
        || key == QLL("DYLD_FRAMEWORK_PATH")
#  else
        || key == QLL("LD_LIBRARY_PATH")
        || key == QLL("DISPLAY")
#  endif
#endif
        ;
    return useVariable;
}

// CASE	continuation-indent-directives.h
class Commands
{
public:
    static QStringList removeCommand()
    {
#if defined(Q_OS_LINUX)
        return QStringList() << QLL("ip") << QLL("addr") << QLL("del")
            << (QLatin1String(address()) + QLL("/16")) << QLL("dev") << QLL("lo");
#elif defined(Q_OS_UNIX)
        return QStringList() << QLL("ifconfig") << QLL("lo0") << QLL("-alias")
            << QLatin1String(address());
#elif defined(Q_OS_WIN)
        return QStringList() << QLL("netsh") << QLL("interface") << QLL("ipv4") << QLL("delete")
            << QLL("address") << QLL("name=Loopback Pseudo-Interface 1")
            << (QLL("address=") + QLatin1String(address()));
#else
        return QStringList();
#endif
    }
};

// CASE	continuation-indent-nested.cpp
bool pick(int alpha, int beta, bool gamma)
{
    if ((alpha
            == beta)
        && gamma
    ) {
        return true;
    }
    if ((alpha
            || beta)
        && gamma
    ) {
        return true;
    }
    if (alpha
            == beta
    ) {
        return true;
    }
    return false;
}

// CASE	control-braces-inline-range-body.cpp
void run(bool ready)
{
    if (ready) start(
        value);
}

// CASE	control-braces-inline-range-body.h
void run(bool ready)
{
    if (ready) start(
        value);
}

// CASE	control-braces-inline-range-head.cpp
void run(bool ready)
{
    if (ready) start(
        value);
}

// CASE	control-braces-inline-range-head.h
void run(bool ready)
{
    if (ready) start(
        value);
}

// CASE	control-braces-joined-else.cpp
void choose()
{
    if (first) {
        accept();
    } else if (second) {
        defer();
    } else {
        reject();
    }
    if (ready) {
        start();
    } else {
        stop();
    }
    if (cached) {
        useCache();
    }
    // Keep this explanation with the alternative.
    else {
        fetch();
    }
}

// CASE	control-braces-nested-inline.cpp
bool process(bool *success)
{
    if (invalid()) {
        if (success) {
            *success = false;
        }
        return false;
    }
    if (retry()) {
        if (enabled()) {
            retryWork();
        }
        finish();
    }
    return true;
}

// CASE	control-braces-range-body.cpp
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// CASE	control-braces-range-else-body.cpp
void run(bool ready)
{
    if (ready)
        start();
    else
        stop();
}

// CASE	control-braces-range-else-head.cpp
void run(bool ready)
{
    if (ready)
        start();
    else
        stop();
}

// CASE	control-braces-range-head.cpp
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// CASE	control-braces-range-whole.cpp
void linkLocalIPv4()
{
    if (addresses.isEmpty()) {
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");
    }

    runSockets();
}

// CASE	control-braces-ten-inner.cpp
void batch()
{
    if (active) {
        if (flag0) {
            handle0();
        }
        if (flag1) {
            handle1();
        }
        if (flag2) {
            handle2();
        }
        if (flag3) {
            handle3();
        }
        if (flag4) {
            handle4();
        }
        if (flag5) {
            handle5();
        }
        if (flag6) {
            handle6();
        }
        if (flag7) {
            handle7();
        }
        if (flag8) {
            handle8();
        }
        if (flag9) {
            handle9();
        }
        finish();
    }
}

// CASE	init-list-range.h
class Widget
{
public:
    inline Widget() : m_ready(false), m_state(Idle),
                      m_mode(Fast), m_error(None)
                      , m_count(-1)
    { }

private:
    enum State { Idle, Busy };
    enum Mode { Fast, Slow };
    enum Error { None, Lost };

    bool m_ready;
    State m_state;
    Mode m_mode;
    Error m_error;
    int m_count;
};

// CASE	init-list.h
class Widget
{
public:
    inline Widget() : m_ready(false), m_state(Idle)
                      , m_mode(Fast), m_error(None)
                      , m_count(-1)
    { }

private:
    enum State { Idle, Busy };
    enum Mode { Fast, Slow };
    enum Error { None, Lost };

    bool m_ready;
    State m_state;
    Mode m_mode;
    Error m_error;
    int m_count;
};

// CASE	line-join-long.cpp
void run(Document *document, int open, int close)
{
    if (open > 0
        && close > open
    ) {
        apply(document,
              text);
    }
    const int total = open
        + close;
    const int side = open > close
        ? open
        : close;
    const QString message = QString(QLL("first part of the message, "
                                        "second part of it"));
    document->replace(document->lineStart(line), current.size(),
        indentation + suffix + prefix);
}

// CASE	namespace-braces.cpp
using namespace std;
namespace fs = std::filesystem;

namespace Alpha {
void a();
} // Namespace `Alpha`.

namespace Beta {
void b();
} // Namespace `Beta`.

namespace {
int c;
}

namespace Gamma::Delta {
void d();
} // Namespace `Gamma::Delta`.

namespace Outer {
namespace Inner {
void e();
} // Namespace `Inner`.
} // Namespace `Outer`.

namespace Epsilon {
void f();
} // End of the Epsilon helpers

namespace Zeta {
void g();
// Closes the Zeta helpers.
}

namespace Eta {
void h();
} // Namespace `Eta`.

namespace Theta {
void i();
} // Namespace `Theta`.

// CASE	operator-first-join-guards.cpp
Widget::Widget(int a,
               int b
)
    : m_a(a)
{
    if (isReady(a,
            b
        )
    )
        ++count;
    const int total = sum(a,
        b
    ) + 1;
}

// CASE	operator-first-range.cpp
bool isReady(bool open, bool loaded, bool valid)
{
    return open &&
        loaded
        && valid;
}

// CASE	operator-first.cpp
bool isReady(bool open, bool loaded, bool valid)
{
    return open
        && loaded
        && valid;
}

// CASE	operator-join-closings-eof.cpp
int value = build(
    input
) + 1;
// CASE	operator-join-closings-range.cpp
void result()
{
    target = calculate(
        value
    )
        + other;
}

// CASE	operator-join-closings-selected.cpp
void result()
{
    target = calculate(
        value
    ) + other;
}

// CASE	operator-join-closings.cpp
void operators()
{
    target = calculate(
        value
    ) + other;
    target = calculate(
        value
    ) - other;
    target = calculate(
        value
    ) * other;
    target = calculate(
        value
    ) / other;
    target = calculate(
        value
    ) % other;
    target = calculate(
        value
    ) << other;
    target = calculate(
        value
    ) >> other;
    target = calculate(
        value
    ) & other;
    target = calculate(
        value
    ) | other;
    target = calculate(
        value
    ) ^ other;
    target = calculate(
        value
    )
        && other;
    target = calculate(
        value
    )
        || other;
    target = calculate(
        value
    ) == other;
    target = calculate(
        value
    ) != other;
    target = calculate(
        value
    ) < other;
    target = calculate(
        value
    ) > other;
    target = calculate(
        value
    ) <= other;
    target = calculate(
        value
    ) >= other;
    target = calculate(
        value
    ) = other;
    target = calculate(
        value
    ) += other;
    target = calculate(
        value
    ) -= other;
    target = calculate(
        value
    ) *= other;
    target = calculate(
        value
    ) /= other;
    target = calculate(
        value
    ) %= other;
    target = calculate(
        value
    ) &= other;
    target = calculate(
        value
    ) |= other;
    target = calculate(
        value
    ) ^= other;
    target = calculate(
        value
    ) <<= other;
    target = calculate(
        value
    ) >>= other;
    target = calculate(
        value
    ).member;
    target = calculate(
        value
    )->member;
    target = calculate(
        value
    ).*member;
    target = calculate(
        value
    )->*member;
    target = calculate(
        value
    )++;
    target = calculate(
        value
    )--;
    target = values[
        index
    ] + other;
    target = Value{
        value
    } + other;
    target = calculate(
        nested(
            value
    )) + other;
    target = predicate(
        value
    ) ? first : second;
    target = condition ? calculate(
        value
    ) : other;
}

// CASE	operator-wrap-arrow.cpp
void chooseActiveConfig()
{
    serviceConfig = engineManager
        ->sessionManager()
        ->configurationFromIdentifier(QLL("dummy-service-network"));
}

// CASE	operator-wrap-assign-compare.cpp
void pick()
{
    QHash<QString, QNetworkConfigurationPrivatePointer > &table =
        typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice;
    SomethingShort &flag = typeNameIsVeryVeryLongInThisCase
        == QNetworkConfiguration::UserChoiceValue;
}

// CASE	operator-wrap-assignment-all.cpp
void workerErrors()
{
    {
        {
            {
                {
                    {
                        results[index].error = QString(QLL("worker failed: %1"))
                            .arg(process->errorString());
                        results[index].error +=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error -=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error *=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error /=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error %=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error &=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error |=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error ^=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error <<=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error >>=
                            QString(QLL("failure")).arg(process->errorString());
                    }
                }
            }
        }
    }
}

// CASE	operator-wrap-assignment.cpp
void workerErrors()
{
    {
        {
            {
                {
                    {
                        results[index].error = QString(QLL("worker failed: %1"))
                            .arg(process->errorString());
                        results[index].error +=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error -=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error *=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error /=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error %=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error &=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error |=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error ^=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error <<=
                            QString(QLL("failure")).arg(process->errorString());
                        results[index].error >>=
                            QString(QLL("failure")).arg(process->errorString());
                    }
                }
            }
        }
    }
}

// CASE	operator-wrap-chain.cpp
QByteArray lastField(const QByteArray & record)
{
    if (record.isEmpty()) {
        return QByteArray();
    }
    {
        {
            const QByteArray field = record.mid(
                record.indexOf(
                    ':',
                    record.indexOf(
                        ':',
                        record.indexOf(':', record.indexOf(':') + 1) + 1
                    ) + 1
                ) + 1
            );
            return field;
        }
    }
}

// CASE	operator-wrap-dot-all.cpp
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager()
        .configurationFromIdentifier(QLL("dummy-service-network"));
}

void chainedCalls()
{
    value = source.first().second()
        .configurationFromIdentifier(QLL("dummy-service-network"));
    const QString shortValue = source.first().second();
}

// CASE	operator-wrap-dot.cpp
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager()
        .configurationFromIdentifier(QLL("dummy-service-network"));
}

void chainedCalls()
{
    value = source.first().second()
        .configurationFromIdentifier(QLL("dummy-service-network"));
    const QString shortValue = source.first().second();
}

// CASE	ternary-wrap-cast.cpp
int pick(int someVeryLongExpressionNameHere, int anotherQuiteLongOperand)
{
    const int value =
        static_cast<int>(someVeryLongExpressionNameHere) + anotherQuiteLongOperand
        ? yesValue : noValue;
    return value;
}

// CASE	ternary-wrap-head.cpp
void pick(int type, const Item & configuration)
{
    QHash<QString, QNetworkConfigurationPrivatePointer > &table =
        type == QNetworkConfiguration::UserChoice
        ? userChoiceConfigurations : snapConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer > &other =
        typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice
        ? userChoiceConfigurations : snapConfigurations;
    SomethingShort &shortTable = typeNameIsVeryVeryLongInThisCase
            == QNetworkConfiguration::UserChoice
        ? userChoiceConfigurations : snapConfigurations;
    const bool usable = configuration.isValid()
        || configuration.state() == Discovered
        ? true : fallbackCheck(configuration);
}

// CASE	body-braces-off.h
class DummyEngine
{
public:
    int count() const { return m_count; }
    int total() const { return count(); }

private slots:
    void doSend(const QByteArray &verb, const QByteArray &id)
    { if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
        m_sock->write(verb + ' ' + id + '\n');
        m_sock->flush();
    } }

    void flush()
    { m_sock->write(m_buffer);
        m_sock->flush(); }

    void reset()
    {
        if (m_sock) {
            m_sock->abort();
        }
    }

    void send(const QByteArray &data)
    { m_sock->write(QByteArray(data.constData(),
        data.size())
    ); }

private:
    QTcpSocket *m_sock;
    int m_count;
};

namespace Fake { void touch() { if (m_count) { ++m_count; }
} }

// CASE	body-braces-header.h
class DummyEngine
{
public:
    int count() const { return m_count; }
    int total() const { return count(); }

private slots:
    void doSend(const QByteArray &verb, const QByteArray &id)
    {
        if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
            m_sock->write(verb + ' ' + id + '\n');
            m_sock->flush();
        }
    }

    void flush()
    { m_sock->write(m_buffer);
        m_sock->flush(); }

    void reset()
    {
        if (m_sock) {
            m_sock->abort();
        }
    }

    void send(const QByteArray &data)
    {
        m_sock->write(QByteArray(data.constData(),
            data.size())
        );
    }

private:
    QTcpSocket *m_sock;
    int m_count;
};

namespace Fake {
void touch() { if (m_count) { ++m_count; }
}
}

// CASE	body-braces-source.cpp
class DummyEngine
{
public:
    int count() const { return m_count; }
    int total() const { return count(); }

private slots:
    void doSend(const QByteArray &verb, const QByteArray &id)
    { if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
        m_sock->write(verb + ' ' + id + '\n');
        m_sock->flush();
    } }

    void flush()
    { m_sock->write(m_buffer);
        m_sock->flush(); }

    void reset()
    {
        if (m_sock) {
            m_sock->abort();
        }
    }

    void send(const QByteArray &data)
    { m_sock->write(QByteArray(data.constData(),
        data.size())
    ); }

private:
    QTcpSocket *m_sock;
    int m_count;
};

namespace Fake { void touch() { if (m_count) { ++m_count; }
} }

// CASE	line-join-logical.cpp
void inspect()
{
    if ((check(item
    ))
        || ready) {}
}

// CASE	line-join-member-access.cpp
void inspect()
{
    send(value.name(), pointer->label());
}

// CASE	line-join.cpp
void run(Document *document, int open, int close)
{
    const QByteArray text = sourceText(document, open + 1, close - 1);
    if (open > 0 && close > open) {
        apply(document, text);
    }
    const int total = open + close;
    const int side = open > close ? open : close;
    const QString message = QString(QLL("first part of the message, "
                                        "second part of it"));
    document->replace(
        document->lineStart(line),
        current.size(),
        indentation + suffix + prefix
    );
}

// CASE	member-chain-arguments.cpp
void describe()
{
    output(
        "category",
        String("A long fictional description with two placeholders: %1 and %2")
            .replace(firstValue())
            .replace(secondValue()),
        String("Another invented description for a single placeholder: %1")
            .replace(value()),
        String("The result has %1 items and took %2 seconds")
            .replace(counter->size())
            .replace(timer.elapsed()),
        String("The record named %1 is currently selected")
            .replace(recordName()),
        String("Values %1 and %2 are ready for processing")
            .replace(first ? 'A' : 'B')
            .replace(secondName())
    );
}

// CASE	member-chain-literals.cpp
void report()
{
    log(
        context,
        location,
        "notice",
        String(Text("First fragment gives a brief introduction; "
            "second fragment adds some extra detail; "
            "third fragment completes the message.")).replace(item)
    );
}

// CASE	member-chain-ternary.cpp
void report()
{
    log(
        "notice",
        String(Text("A fictional description of the %1 and %2 fields"))
            .replace(mode == "short-format" ? Text("compact-value")
                                            : Text("extended-value")
            )
            .replace(fieldName())
    );
}

// CASE	operator-wrap-dot-any.cpp
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager()
        .configurationFromIdentifier(QLL("dummy-service-network"));
}

// CASE	operator-wrap-logical-call.cpp
bool matches()
{
    return acceptsToken(next)
        || ((next == DOT || next == ARROW)
            && hasJoinedExpression(document, before))
        || document->token(before) == EQ;
}

// CASE	operator-wrap-priority-assignment.cpp
void collect()
{
    const QStringList documentLines = QString::fromUtf8(payload.readAll())
        .split(QLatin1Char('\n'));
}

// CASE	operator-wrap-priority-long-suffix.cpp
void collect()
{
    const QString displayName = (root.path()
            + separator()
            + component
        ).withDetailedPresentation()
        .withAdditionalFormattingAndLongOptionsForDisplay();
}

// CASE	operator-wrap-priority.cpp
void collect()
{
    const QString fullName = (root.path()
            + QLatin1Char('/')
            + pieces.at(index)
        ).toLower();
    const QStringList documentLines = QString::fromUtf8(payload.readAll())
        .split(QLatin1Char('\n'));
}

// CASE	short-call-adjacent-literals.cpp
void announce()
{
    publish("note", Message(Text("An invented opening with several details; "
            "another fictional sentence adds information; "
            "the last sentence completes the message.")).substitute(value));
}

// CASE	ternary-priority.cpp
void present()
{
    emitText(
        "case-a",
        Message("A made-up description with placeholders %1 and %2")
            .insert(
                theExtremelyLongComparisonValueForAlphaExtraLonger == "x" ? a
                                                                          : b
            )
            .insert(value())
    );
    emitText(
        "case-b",
        Message("A made-up description with placeholders %1 and %2")
            .insert(
                theExtremelyLongComparisonValueForBetaWithExtraCharacters == "x"
                    ? Text("a") : Text("b")
            )
            .insert(value())
    );
    emitText(
        "case-c",
        Message("A made-up description with placeholders %1 and %2")
            .insert(
                theExtremelyLongComparisonValueForGammaWithExtraChars == "x"
                    ? Text("an invented long positive branch for this case")
                    : Text("an invented long negative branch for this case")
            )
            .insert(value())
    );
    emitText(
        "multi",
        Message("A made-up description with placeholders %1 and %2")
            .insert(longPredicateWithMeaningfulName == "x"
                    ? Text("positiveValue") : Text("negativeValue"),
                other == "z" ? Text("y") : Text("n"),
                comparisonIdentifierForThirdVeryLong == "r" ? Text("p")
                                                            : Text("q")
            )
            .insert(value())
    );
}

// CASE	member-single-property.cpp
void inspect()
{
    const int width = parser->configurationAt(
        parser->symbolFor(currentItem).position
    ).maximumWidth;
}
// CASE	member-inside-check.cpp
void inspect()
{
    VERIFY(output.selectItems(
        StringList() << QLL("first-option") << QLL("another-option"),
        &error
    ));
}
// CASE	comparison-operand-call.cpp
void inspect()
{
    if (position + widthOf(buffer) <= registry->configurationAt(
            registry->symbolFor(entry).offset
        ).maximumWidth
        || flags.isEmpty()
    ) {
        consume();
    }
}
// CASE	comparison-single-member.cpp
void inspect()
{
    if (accepted
        || source->positionOf(currentParcel.first())
            > destination->positionOf(currentParcel.last())
    ) {
        consume();
    }
}
// CASE	subchain-after-multiple-calls.cpp
void inspect()
{
    const int answer = service->lookup(
        record->descriptorFor(currentObject).position,
        fallback
    ).width(
        firstDetailedMeasurementForThePackage,
        secondDetailedMeasurementForThePackage
    ).detail
        .more;
}
// CASE	args-wrap-preserve-condition.cpp
void prepare()
{
    if (ready()) {
        Spacing spacing = Settings::toSpacing(
            hasAlternate ? alternate : ordinaryValue,
            ordinaryValue
        );
    }
}

// CASE	condition-count-over.cpp
void dispatch()
{
    if (routeReady()
        && parcelIsAvailable()
        && destinationAcceptsDelivery()
    ) {
        send();
    }
}
// CASE	condition-count-under.cpp
void dispatch()
{
    if (routeReady() && parcelIsAvailable() && destinationAcceptsDelivery()) {
        send();
    }
}
// CASE	condition-preserve-policy.cpp
void dispatch()
{
    if (parcelReady()
        && destinationReady()
    ) {
        send();
    }
}
// CASE	numeric-operator-single.cpp
void inspect()
{
    const int result = substantialMeasurementForDrawing + 1;
}
// CASE	numeric-operator-chain.cpp
void inspect()
{
    const int result = substantialMeasurementForDrawing
        + 1
        + adjustment
        + 2;
}
// CASE	numeric-operator-separate-portions.cpp
void inspect()
{
    stream << substantialMeasurementForDrawing
        + 1
        + adjustment
        << spare + reserve;
}
// CASE	comparison-continuation-indent.cpp
void inspect()
{
    if (index + offset
            > maximumPermittedSequenceLengthForThisRequest
        || ready()
    ) {
        consume();
    }
}
// CASE	numeric-operator-mixed.cpp
void inspect()
{
    stream << substantialMeasurementForDrawing
        * 2
        + adjustment
        + 0x10;
}
// CASE	subchain-threshold-never.cpp
void inspect()
{
    const auto item = registry->findRecord(
        service->locateItem(currentObject).offset
    ).firstMember.extremelyVerbosePropertyNameForDelivery;
}
// CASE	subchain-maximum-never.cpp
void inspect()
{
    const auto item = registry->findRecord(
        service->locateItem(currentObject).offset
    ).firstMember.extraordinarilyVerbosePropertyNameForThisSpecificDeliveryRecord;
}
// CASE	subchain-threshold-wrap-begin.cpp
void inspect()
{
    const auto item = registry->findRecord(
        service->locateItem(currentObject).offset
    ).firstMember
        .extremelyVerbosePropertyNameForDelivery;
}
// CASE	subchain-threshold-wrap-end.cpp
void inspect()
{
    const auto item = registry->findRecord(
        service->locateItem(currentObject).offset
    ).firstMember.extremelyVerbosePropertyNameForDelivery;
}
// CASE	subchain-maximum-wrap-end.cpp
void inspect()
{
    const auto item = registry->findRecord(
        service->locateItem(currentObject).offset
    ).firstMember
        .extraordinarilyVerbosePropertyNameForThisSpecificDeliveryRecord;
}
// CASE	subchain-always.cpp
void inspect()
{
    const int answer = service->lookup(
        record->descriptorFor(currentObject).position
    ).width
        .detail;
}
// CASE	subchain-never.cpp
void inspect()
{
    const int answer = service->lookup(
        record->descriptorFor(currentObject).position
    ).width.detail;
}
// CASE	subchain-wrap-begin.cpp
void inspect()
{
    const int answer = service->lookup(
        record->descriptorFor(currentObject).position
    ).width.detail;
}
// CASE	subchain-wrap-end.cpp
void inspect()
{
    const int answer = service->lookup(
        record->descriptorFor(currentObject).position
    ).width.detail;
}
// CASE	operator-wrap-first-call.cpp
void append()
{
    output << Packet(
        message.convertToInteger(),
        message.beginsWith(Text("sample"))
    );
}

void appendMore()
{
    output << Packet(
        message.convertToInteger(),
        message.beginsWith(Text("sample"))
    ) << finalItem;
}

// CASE	operator-wrap-first-call-condition.cpp
void append()
{
    output << Packet(
        message.convertToInteger(),
        message.beginsWith(Text("sample"))
    );
}

void appendMore()
{
    output << Packet(
        message.convertToInteger(),
        message.beginsWith(Text("sample"))
    ) << finalItem;
}

// CASE	param-wrap-condition.cpp
void describe(
        int firstVeryLongParameter,
        int secondVeryLongParameter,
        int thirdVeryLongParameter,
        int fourthVeryLongParameter);

// CASE	args-wrap-whole-last-config.cpp
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(
                line.indexOf(
                    ' ',
                    line.indexOf(
                        ' ',
                        ' ',
                        ' ',
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1,
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1,
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1
                    ) + 1
                ) + 1
            );
        }
    }
}

// CASE	args-wrap-condition.cpp
void dispatch()
{
    dispatchWithAReasonablyLongFunctionName(firstValue, secondValue, thirdValue, fourthValue);
    shortCall(firstExtremelyLongValue, secondExtremelyLongValue, thirdExtremelyLongValue, fourthExtremelyLongValue);
    dispatchWithAnIntentionallyLongNameToReachTheNarrowRemainingSpace(
        firstValue,
        secondValue,
        thirdValue,
        fourthValue
    );
}

// CASE	operator-wrap-binary-head.cpp
void record()
{
    results[index].error
        << buildResult().withDetails(details);
    results[index].error
        == buildResult().withDetails(details);
    results[index].veryLongErrorMemberName
        .detailValue
        << buildResult().withDetails(details);
}

// CASE	operator-wrap-assignment-threshold.cpp
void record()
{
    results[index].error = makeValue()
        .withExtendedDescription(details);
    results[index]
        .veryLongErrorMemberName = makeValue()
            .withExtendedDescription(details);
}

// CASE	operator-join-closings-wide.cpp
void result()
{
    target = calculate(
        value
    )
        + aValueWhoseNameIsTooLongForTheClosingLine;
}

// CASE	condition-count-under-limit.cpp
void dispatch()
{
    if (routeReady() && parcelIsAvailable()
        && destinationAcceptsDelivery()
    ) {
        send();
    }
}
// CASE	condition-per-line.cpp
void dispatch()
{
    if (ready && (kind == CLASS || kind == STRUCT)
        && parcelIsAvailable()
    ) {
        send();
    }
}
// CASE	condition-per-pair.cpp
void dispatch()
{
    if (ready
        && (kind == CLASS || kind == STRUCT)
        && parcelIsAvailable()
    ) {
        send();
    }
}
// CASE	condition-per-bundle.cpp
void dispatch()
{
    if (ready
        && (kind == CLASS
            || kind == STRUCT)
        && parcelIsAvailable()
    ) {
        send();
    }
}
// CASE	condition-per-bundle-call.cpp
void dispatch()
{
    if (ready && valid
        && checkAll(alpha || beta || gamma)
    ) {
        send();
    }
}
// CASE	condition-per-root.cpp
void dispatch()
{
    if (ready
        && valid
        && checkAll(alpha || beta || gamma)
    ) {
        send();
    }
}
// CASE	condition-closers-line.cpp
void move(const Edit &edit, int end, int offset, bool isStart)
{
    for (;;) {
        if (end < offset
            || (end == offset && (edit.length > 0 || isStart))
        ) {
            moved();
        }
    }
}
// CASE	condition-closers-bundle.cpp
void move(const Edit &edit, int end, int offset, bool isStart)
{
    for (;;) {
        if (end < offset
            || (end == offset
                && (edit.length > 0
                    || isStart)
            )
        ) {
            moved();
        }
    }
}
// CASE	operator-first-access-label.cpp
class Matcher : public Base
{
public:
    bool match(const QString &value, QString *matched) const Q_DECL_OVERRIDE
    {
        if (value.isEmpty()) {
            return false;
        } else {
            check(value);
        }
        *matched = value;
        return true;
    }
};
// CASE	operator-wrap-before-call.cpp
void Runner::prepare(Config &config)
{
    config.snapshotManifest = this->m_snapshotDirectory->path()
        + QLL("/manifest");
}
// CASE	operator-wrap-member-call.cpp
void Runner::prepare(const Config &config, int i)
{
    for (;;) {
        const QString original = QDir::cleanPath(QFileInfo(
                config.scheduledFiles.at(i)
            ).absoluteFilePath()
        );
    }
}
// CASE	param-wrap-signature-at.cpp
static bool isNumberMathChain(
        const FormatDocument *document, int first, int last)
{
    return first < last;
}
