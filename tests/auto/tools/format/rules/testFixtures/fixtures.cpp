// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

// CASE	args-wrap-aligned-chain.cpp	args-wrap		1
// BEFORE
void run(Document *document, int open, int close)
{
    const QByteArray text = sourceText(document,
                                       open + 1,
                                       close - 1);
}

// AFTER
void run(Document *document, int open, int close)
{
    const QByteArray text = sourceText(
        document,
        open + 1,
        close - 1
    );
}

// END
// CASE	args-wrap-all-items-all.cpp			1
// BEFORE
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(line.indexOf(' ', line.indexOf(' ', line.indexOf('x'), line.indexOf(' ', line.indexOf(' ') + 1) + 1) + 1) + 1);
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-all-items-condition.cpp	args-wrap		1
// BEFORE
void update()
{
    {
        {
            {
                {
                    c->m_linkLocalIPv4 = (val.compare(QLL("true"), Qt::CaseInsensitive) == 0
                        || val.toInt() != 0);
                }
            }
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-all-items.cpp	args-wrap		1
// BEFORE
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(line.indexOf(' ', line.indexOf(' ', line.indexOf('x'), line.indexOf(' ', line.indexOf(' ') + 1) + 1) + 1) + 1);
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-chain.cpp	args-wrap		1
// BEFORE
QByteArray lastField(const QByteArray & record)
{
    if (record.isEmpty()) {
        return QByteArray();
    }
    {
        {
            const QByteArray field = record.mid(record.indexOf(':', record.indexOf(':', record.indexOf(':', record.indexOf(':') + 1) + 1) + 1) + 1);
            return field;
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-condition.cpp	args-wrap		1
// BEFORE
// Format rule: structure.wrapArgsOver=argCount(0) > 3 && spaceRemaining <= 18.75% && argCount(1, 0) >= -1
void dispatch()
{
    dispatchWithAReasonablyLongFunctionName(firstValue, secondValue, thirdValue, fourthValue);
    shortCall(firstExtremelyLongValue, secondExtremelyLongValue, thirdExtremelyLongValue, fourthExtremelyLongValue);
    dispatchWithAnIntentionallyLongNameToReachTheNarrowRemainingSpace(firstValue, secondValue, thirdValue, fourthValue);
}

// AFTER
// Format rule: structure.wrapArgsOver=argCount(0) > 3 && spaceRemaining <= 18.75% && argCount(1, 0) >= -1
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

// END
// CASE	args-wrap-logical-priority.cpp	args-wrap		1
// BEFORE
bool matches()
{
    return acceptsToken(next)
        || ((next == DOT || next == ARROW) && hasJoinedExpression(document, before))
        || document->token(before) == EQ;
}

bool alternate()
{
    return cachedResult || findAlternateRecord(firstCandidate, secondCandidate);
}

bool validate()
{
    return isAvailable && validateExtraordinarilyLongArguments(firstExcessivelyLongCandidateName, secondExcessivelyLongCandidateName);
}

// AFTER
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

// END
// CASE	args-wrap-matched-closings.cpp	args-wrap		1
// BEFORE
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

// AFTER
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

// END
// CASE	args-wrap-operator-lines.cpp	args-wrap		1
// BEFORE
void sum(Log *log, const QList<int > & values, int count, int total)
{
    log->line(
        QString::number(count).rightJustified(8, QLatin1Char(' ')).toUpper().trimmed(),
        values[1] + values[2]
        + count
            + total
    );
}

// AFTER
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

// END
// CASE	args-wrap-preserve-condition.cpp			1
// BEFORE
// Format rule: structure.target=any
// Format rule: structure.wrapArgsOver=argCount(0) > 3 && (spaceRemaining <= 18.75% || isSignature)
void prepare()
{
    if (ready()) {
        Spacing spacing = Settings::toSpacing(
            hasAlternate ? alternate : ordinaryValue,
            ordinaryValue
        );
    }
}

// AFTER
// Format rule: structure.target=any
// Format rule: structure.wrapArgsOver=argCount(0) > 3 && (spaceRemaining <= 18.75% || isSignature)
void prepare()
{
    if (ready()) {
        Spacing spacing = Settings::toSpacing(
            hasAlternate ? alternate : ordinaryValue,
            ordinaryValue
        );
    }
}

// END
// CASE	args-wrap-range-end.cpp	args-wrap		1
// BEFORE
void report(Log *log, const QString & suffix, const QList<int > & values, int index)
{
    log->line(QString::fromLatin1("the first part of a rather long message to the user and ") + suffix, values[index + 1] + values[index + 2] + values[index + 3] + values[index + 4] + values[index + 5], u8"another long piece of text that keeps on going for quite a while longer" + suffix, "a plain literal piece of text that is also long enough to wrap right here" + suffix);
}

// AFTER
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

// END
// CASE	args-wrap-single-broken.cpp	args-wrap		1
// BEFORE
QByteArray lastField(const QByteArray & record)
{
    {
        {
            const QByteArray field = record.mid(
                record.indexOf(
                    ':',
                    record.indexOf(':', record.indexOf(':', record.indexOf(':') + 1)
                        + 1
                    )
                        + 1
                )
                    + 1
            );
            return field;
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-string-end.cpp	args-wrap		1
// BEFORE
void report(Log *log, const QString & prefix, const QString & suffix, int count)
{
    log->line(
        QString::number(count).rightJustified(8, QLatin1Char(' ')).toUpper().trimmed(),
        prefix + "a literal piece of text that is long enough to wrap"
            + suffix
    );
}

// AFTER
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

// END
// CASE	args-wrap-string-lines.cpp	args-wrap		1
// BEFORE
void writeAll()
{
    const QString file = this->writeFile(
        QString(QLL("rewrite/a.cpp")),
        "void f()\n{\n    // The tool waits for the server to answer\n"
        "    // each request in turn.\n    g(first,\n      second);\n}\n"
    );
}

// AFTER
void writeAll()
{
    const QString file = this->writeFile(
        QString(QLL("rewrite/a.cpp")),
        "void f()\n{\n    // The tool waits for the server to answer\n"
            "    // each request in turn.\n    g(first,\n      second);\n}\n"
    );
}

// END
// CASE	args-wrap-trailing-comma.cpp	args-wrap		1
// BEFORE
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

// AFTER
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

// END
// CASE	args-wrap-whole-last-config.cpp	args-wrap,operator-wrap		1
// BEFORE
// Format rule: structure.wrapArgsOver=6
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(line.indexOf(' ', line.indexOf(' ', ' ', ' ', line.indexOf(' ', line.indexOf(' ') + 1) + 1, line.indexOf(' ', line.indexOf(' ') + 1) + 1, line.indexOf(' ', line.indexOf(' ') + 1) + 1) + 1) + 1);
        }
    }
}

// AFTER
// Format rule: structure.wrapArgsOver=6
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

// END
// CASE	args-wrap-whole-last-over.cpp	args-wrap,operator-wrap		1
// BEFORE
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(line.indexOf(' ', ' ', line.indexOf(' ', line.indexOf(' ') + 1) + 1, line.indexOf(' ', line.indexOf(' ') + 1) + 1) + 1);
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-whole-last-two.cpp	args-wrap,operator-wrap		1
// BEFORE
void parse(const QByteArray & line)
{
    {
        {
            const QByteArray name = line.mid(line.indexOf(' ', line.indexOf(' ', line.indexOf(' ', line.indexOf(' ') + 1) + 1, line.indexOf(' ', line.indexOf(' ') + 1) + 1) + 1) + 1);
        }
    }
}

// AFTER
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

// END
// CASE	args-wrap-whole-last.cpp			1
// BEFORE
QByteArray lastWord(const QByteArray & text)
{
    {
        {
            const QByteArray word = text.mid(text.indexOf(':', text.indexOf(':', text.indexOf(':', text.indexOf(':') + 1) + 1) + 1) + 1);
            return word;
        }
    }
}

// AFTER
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

// END
// CASE	body-braces-header.h	body-braces		1
// BEFORE
// Format rule: structure.bodyBraces=is-header && !is-non-header
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

// AFTER
// Format rule: structure.bodyBraces=is-header && !is-non-header
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

// END
// CASE	body-braces-off.h	body-braces		1
// BEFORE
// Format rule: structure.bodyBraces=false
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

// AFTER
// Format rule: structure.bodyBraces=false
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

// END
// CASE	body-braces-source.cpp	body-braces		1
// BEFORE
// Format rule: structure.bodyBraces=is-header && !is-non-header
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

// AFTER
// Format rule: structure.bodyBraces=is-header && !is-non-header
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

// END
// CASE	body-braces.h	body-braces		1
// BEFORE
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

// AFTER
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

// END
// CASE	condition-wrap-compare-all.cpp			1
// BEFORE
bool isActive(const Item *item)
{
    for (int i = 0; i < 1; ++i) {
        {
            if ((item->value()->state & ItemConfiguration::Active) == ItemConfiguration::Active) {
                return true;
            }
        }
    }
    return false;
}

// AFTER
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

// END
// CASE	condition-wrap-compare-fit.cpp	condition-wrap,continuation-indent		1
// BEFORE
void pick(bool ready)
{
    if (ready && typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::Active) {
        use();
    }
    if (ready && typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoiceValueThatIsLonger) {
        use();
    }
}

// AFTER
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

// END
// CASE	condition-wrap-compare-max.cpp	condition-wrap		1
// BEFORE
int kindOf(const Config & publicConfig)
{
    if (publicConfig.isValid()) {
        return 0;
    } else if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        return 1;
    }
    if (publicConfig.isValid() && publicConfig.type() == Network::Service) {
        return 2;
    }
    return 3;
}

// AFTER
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

// END
// CASE	condition-wrap-compare.cpp	condition-wrap		1
// BEFORE
bool isActive(const Item *item)
{
    for (int i = 0; i < 1; ++i) {
        {
            if ((item->value()->state & ItemConfiguration::Active) == ItemConfiguration::Active) {
                return true;
            }
        }
    }
    return false;
}

// AFTER
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

// END
// CASE	condition-wrap-nested.cpp	condition-wrap		1
// BEFORE
bool pick(int token, int k, const Document *document)
{
    {
        {
            if ((token == LPAREN || token == LBRACK) && document->match(k) > k) {
                return true;
            }
            if ((first == second || third == fourth || fifth == sixth || seventh == eighth) && ready) {
                return false;
            }
        }
    }
    return false;
}

// AFTER
bool pick(int token, int k, const Document *document)
{
    {
        {
            if ((token == LPAREN || token == LBRACK)
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

// END
// CASE	condition-wrap-split.cpp	condition-wrap		1
// BEFORE
void poll(bool hasData, bool isBuffered, int socketType, Engine *engine)
{
    if (!hasData && !isBuffered && socketType == UdpSocketType && engine->isOpen()
        && engine->lastError() == ConnectionRefusedError) {
        engine->reset();
    }
}

// AFTER
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

// END
// CASE	continuation-indent-directives.h	continuation-indent		1
// BEFORE
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

// AFTER
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

// END
// CASE	continuation-indent-nested.cpp	continuation-indent		1
// BEFORE
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

// AFTER
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

// END
// CASE	control-braces-inline-range-body.cpp	control-braces	7,7	1
// BEFORE
void run(bool ready)
{
    if (ready) start(
        value);
}

// AFTER
void run(bool ready)
{
    if (ready) start(
        value);
}

// END
// CASE	control-braces-inline-range-body.h	control-braces	7,7	1
// BEFORE
void run(bool ready)
{
    if (ready) start(
        value);
}

// AFTER
void run(bool ready)
{
    if (ready) start(
        value);
}

// END
// CASE	control-braces-inline-range-head.cpp	control-braces	6,6	1
// BEFORE
void run(bool ready)
{
    if (ready) start(
        value);
}

// AFTER
void run(bool ready)
{
    if (ready) start(
        value);
}

// END
// CASE	control-braces-inline-range-head.h	control-braces	6,6	1
// BEFORE
void run(bool ready)
{
    if (ready) start(
        value);
}

// AFTER
void run(bool ready)
{
    if (ready) start(
        value);
}

// END
// CASE	control-braces-joined-else.cpp	control-braces		1
// BEFORE
void choose()
{
    if (first) accept();
    else if (second) defer();
    else reject();
    if (ready) { start(); }
    else { stop(); }
    if (cached) { useCache(); }
    // Keep this explanation with the alternative.
    else { fetch(); }
}

// AFTER
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

// END
// CASE	control-braces-nested-inline.cpp	control-braces		1
// BEFORE
bool process(bool *success)
{
    if (invalid()) { if (success) *success = false; return false; }
    if (retry()) { if (enabled()) retryWork(); finish(); }
    return true;
}

// AFTER
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

// END
// CASE	control-braces-range-body.cpp	control-braces	7,7	1
// BEFORE
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// AFTER
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// END
// CASE	control-braces-range-else-body.cpp	control-braces	7,7	1
// BEFORE
void run(bool ready)
{
    if (ready)
        start();
    else
        stop();
}

// AFTER
void run(bool ready)
{
    if (ready)
        start();
    else
        stop();
}

// END
// CASE	control-braces-range-else-head.cpp	control-braces	6,6	1
// BEFORE
void run(bool ready)
{
    if (ready)
        start();
    else
        stop();
}

// AFTER
void run(bool ready)
{
    if (ready)
        start();
    else
        stop();
}

// END
// CASE	control-braces-range-head.cpp	control-braces	6,6	1
// BEFORE
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// AFTER
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// END
// CASE	control-braces-range-whole.cpp	control-braces	6,7	1
// BEFORE
void linkLocalIPv4()
{
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");

    runSockets();
}

// AFTER
void linkLocalIPv4()
{
    if (addresses.isEmpty()) {
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");
    }

    runSockets();
}

// END
// CASE	control-braces-ten-inner.cpp	control-braces		1
// BEFORE
void batch()
{
    if (active) { if (flag0) handle0(); if (flag1) handle1(); if (flag2) handle2(); if (flag3) handle3(); if (flag4) handle4(); if (flag5) handle5(); if (flag6) handle6(); if (flag7) handle7(); if (flag8) handle8(); if (flag9) handle9(); finish(); }
}

// AFTER
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

// END
// CASE	init-list-range.h	init-list-lead	8,9	1
// BEFORE
class Widget
{
public:
    inline Widget() : m_ready(false), m_state(Idle),
                      m_mode(Fast), m_error(None),
                      m_count(-1)
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

// AFTER
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

// END
// CASE	init-list.h	init-list-lead		1
// BEFORE
class Widget
{
public:
    inline Widget() : m_ready(false), m_state(Idle),
                      m_mode(Fast), m_error(None),
                      m_count(-1)
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

// AFTER
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

// END
// CASE	line-join-logical.cpp	line-join		1
// BEFORE
// Format rule: structure.target=any
void inspect()
{
    if ((check(
        item
    ))
        || ready
    ) {}
}

// AFTER
// Format rule: structure.target=any
void inspect()
{
    if ((check(item
    ))
        || ready) {}
}

// END
// CASE	line-join-long.cpp	line-join,condition-wrap,args-wrap		1
// BEFORE
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

// AFTER
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

// END
// CASE	line-join-member-access.cpp	line-join		1
// BEFORE
// Format rule: structure.target=any
void inspect()
{
    send(value
        .name(), pointer
        ->label());
}

// AFTER
// Format rule: structure.target=any
void inspect()
{
    send(value.name(), pointer->label());
}

// END
// CASE	line-join.cpp	line-join,condition-wrap,args-wrap		1
// BEFORE
// Format rule: structure.target=any
void run(Document *document, int open, int close)
{
    const QByteArray text = sourceText(document,
                                       open + 1,
                                       close - 1);
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

// AFTER
// Format rule: structure.target=any
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

// END
// CASE	member-chain-arguments.cpp			1
// BEFORE
// Format rule: structure.target=any
void describe()
{
    output(
        "category",
        String("A long fictional description with two placeholders: %1 and %2") .replace(firstValue()) .replace(secondValue()),
        String("Another invented description for a single placeholder: %1").replace(value()),
        String("The result has %1 items and took %2 seconds") .replace(counter->size()).replace(timer.elapsed()),
        String("The record named %1 is currently selected") .replace(recordName()),
        String("Values %1 and %2 are ready for processing") .replace(first ? 'A' : 'B') .replace(secondName())
    );
}

// AFTER
// Format rule: structure.target=any
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

// END
// CASE	member-chain-literals.cpp			1
// BEFORE
// Format rule: structure.target=any
void report()
{
    log(
        context,
        location,
        "notice",
        String(Text("First fragment gives a brief introduction; " "second fragment adds some extra detail; " "third fragment completes the message.")).replace(
            item
        )
    );
}

// AFTER
// Format rule: structure.target=any
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

// END
// CASE	member-chain-ternary.cpp			1
// BEFORE
// Format rule: structure.target=any
void report()
{
    log(
        "notice",
        String(Text("A fictional description of the %1 and %2 fields")) .replace(mode == "short-format" ? Text("compact-value") : Text("extended-value")) .replace(
            fieldName()
        )
    );
}

// AFTER
// Format rule: structure.target=any
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

// END
// CASE	namespace-braces.cpp	namespace-braces		1
// BEFORE
using namespace std;
namespace fs = std::filesystem;

namespace Alpha { void a(); }

namespace Beta {
void b();
} // namespace Beta

namespace {
int c;
}

namespace Gamma::Delta {
void d();
}

namespace Outer { namespace Inner {
void e();
} }

namespace Epsilon {
void f();
} // End of the Epsilon helpers

namespace Zeta {
void g();
// Closes the Zeta helpers.
}

namespace Eta {
void h();
}  // end of namespace eta

namespace Theta {
void i();
} // TODO: split

// AFTER
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

// END
// CASE	operator-first-join-guards.cpp	operator-first		1
// BEFORE
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
    )
        + 1;
}

// AFTER
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

// END
// CASE	operator-first-range.cpp	operator-first	7,8	1
// BEFORE
bool isReady(bool open, bool loaded, bool valid)
{
    return open &&
        loaded &&
        valid;
}

// AFTER
bool isReady(bool open, bool loaded, bool valid)
{
    return open &&
        loaded
        && valid;
}

// END
// CASE	operator-first.cpp	operator-first		1
// BEFORE
bool isReady(bool open, bool loaded, bool valid)
{
    return open &&
        loaded &&
        valid;
}

// AFTER
bool isReady(bool open, bool loaded, bool valid)
{
    return open
        && loaded
        && valid;
}

// END
// CASE	operator-join-closings-eof.cpp	operator-first		0
// BEFORE
int value = build(
    input
)
+ 1;
// AFTER
int value = build(
    input
) + 1;
// END
// CASE	operator-join-closings-range.cpp	operator-first	9,9	1
// BEFORE
void result()
{
    target = calculate(
        value
    )
        + other;
}

// AFTER
void result()
{
    target = calculate(
        value
    )
        + other;
}

// END
// CASE	operator-join-closings-selected.cpp	operator-first	8,9	1
// BEFORE
void result()
{
    target = calculate(
        value
    )
        + other;
}

// AFTER
void result()
{
    target = calculate(
        value
    ) + other;
}

// END
// CASE	operator-join-closings-wide.cpp	operator-first		1
// BEFORE
// Format rule: structure.wrapMax=40
void result()
{
    target = calculate(
        value
    )
        + aValueWhoseNameIsTooLongForTheClosingLine;
}

// AFTER
// Format rule: structure.wrapMax=40
void result()
{
    target = calculate(
        value
    )
        + aValueWhoseNameIsTooLongForTheClosingLine;
}

// END
// CASE	operator-join-closings.cpp	operator-first		1
// BEFORE
void operators()
{
    target = calculate(
        value
    )
        + other;
    target = calculate(
        value
    )
        - other;
    target = calculate(
        value
    )
        * other;
    target = calculate(
        value
    )
        / other;
    target = calculate(
        value
    )
        % other;
    target = calculate(
        value
    )
        << other;
    target = calculate(
        value
    )
        >> other;
    target = calculate(
        value
    )
        & other;
    target = calculate(
        value
    )
        | other;
    target = calculate(
        value
    )
        ^ other;
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
    )
        == other;
    target = calculate(
        value
    )
        != other;
    target = calculate(
        value
    )
        < other;
    target = calculate(
        value
    )
        > other;
    target = calculate(
        value
    )
        <= other;
    target = calculate(
        value
    )
        >= other;
    target = calculate(
        value
    )
        = other;
    target = calculate(
        value
    )
        += other;
    target = calculate(
        value
    )
        -= other;
    target = calculate(
        value
    )
        *= other;
    target = calculate(
        value
    )
        /= other;
    target = calculate(
        value
    )
        %= other;
    target = calculate(
        value
    )
        &= other;
    target = calculate(
        value
    )
        |= other;
    target = calculate(
        value
    )
        ^= other;
    target = calculate(
        value
    )
        <<= other;
    target = calculate(
        value
    )
        >>= other;
    target = calculate(
        value
    )
        .member;
    target = calculate(
        value
    )
        ->member;
    target = calculate(
        value
    )
        .*member;
    target = calculate(
        value
    )
        ->*member;
    target = calculate(
        value
    )
        ++;
    target = calculate(
        value
    )
        --;
    target = values[
        index
    ]
        + other;
    target = Value{
        value
    }
        + other;
    target = calculate(
        nested(
            value
    ))
        + other;
    target = predicate(
        value
    )
        ? first : second;
    target = condition ? calculate(
        value
    )
        : other;
}

// AFTER
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

// END
// CASE	operator-wrap-arrow.cpp	operator-wrap		1
// BEFORE
void chooseActiveConfig()
{
    serviceConfig = engineManager->sessionManager()->configurationFromIdentifier(QLL("dummy-service-network"));
}

// AFTER
void chooseActiveConfig()
{
    serviceConfig = engineManager
        ->sessionManager()
        ->configurationFromIdentifier(QLL("dummy-service-network"));
}

// END
// CASE	operator-wrap-assign-compare.cpp	operator-wrap		1
// BEFORE
void pick()
{
    QHash<QString, QNetworkConfigurationPrivatePointer > &table = typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice;
    SomethingShort &flag = typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoiceValue;
}

// AFTER
void pick()
{
    QHash<QString, QNetworkConfigurationPrivatePointer > &table =
        typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice;
    SomethingShort &flag = typeNameIsVeryVeryLongInThisCase
        == QNetworkConfiguration::UserChoiceValue;
}

// END
// CASE	operator-wrap-assignment-all.cpp			1
// BEFORE
void workerErrors()
{
    {
        {
            {
                {
                    {
                        results[index].error = QString(QLL("worker failed: %1")).arg(process->errorString());
                        results[index].error += QString(QLL("failure")).arg(process->errorString());
                        results[index].error -= QString(QLL("failure")).arg(process->errorString());
                        results[index].error *= QString(QLL("failure")).arg(process->errorString());
                        results[index].error /= QString(QLL("failure")).arg(process->errorString());
                        results[index].error %= QString(QLL("failure")).arg(process->errorString());
                        results[index].error &= QString(QLL("failure")).arg(process->errorString());
                        results[index].error |= QString(QLL("failure")).arg(process->errorString());
                        results[index].error ^= QString(QLL("failure")).arg(process->errorString());
                        results[index].error <<= QString(QLL("failure")).arg(process->errorString());
                        results[index].error >>= QString(QLL("failure")).arg(process->errorString());
                    }
                }
            }
        }
    }
}

// AFTER
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

// END
// CASE	operator-wrap-assignment-threshold.cpp	operator-wrap		1
// BEFORE
// Format rule: structure.wrapBegin=40
// Format rule: structure.wrapMax=60
void record()
{
    results[index].error = makeValue().withExtendedDescription(details);
    results[index].veryLongErrorMemberName = makeValue().withExtendedDescription(details);
}

// AFTER
// Format rule: structure.wrapBegin=40
// Format rule: structure.wrapMax=60
void record()
{
    results[index].error = makeValue()
        .withExtendedDescription(details);
    results[index]
        .veryLongErrorMemberName = makeValue()
            .withExtendedDescription(details);
}

// END
// CASE	operator-wrap-assignment.cpp	operator-wrap		1
// BEFORE
void workerErrors()
{
    {
        {
            {
                {
                    {
                        results[index].error = QString(QLL("worker failed: %1")).arg(process->errorString());
                        results[index].error += QString(QLL("failure")).arg(process->errorString());
                        results[index].error -= QString(QLL("failure")).arg(process->errorString());
                        results[index].error *= QString(QLL("failure")).arg(process->errorString());
                        results[index].error /= QString(QLL("failure")).arg(process->errorString());
                        results[index].error %= QString(QLL("failure")).arg(process->errorString());
                        results[index].error &= QString(QLL("failure")).arg(process->errorString());
                        results[index].error |= QString(QLL("failure")).arg(process->errorString());
                        results[index].error ^= QString(QLL("failure")).arg(process->errorString());
                        results[index].error <<= QString(QLL("failure")).arg(process->errorString());
                        results[index].error >>= QString(QLL("failure")).arg(process->errorString());
                    }
                }
            }
        }
    }
}

// AFTER
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

// END
// CASE	operator-wrap-binary-head.cpp	operator-wrap		1
// BEFORE
// Format rule: structure.wrapBegin=40
// Format rule: structure.wrapMax=50
void record()
{
    results[index].error << buildResult().withDetails(details);
    results[index].error == buildResult().withDetails(details);
    results[index].veryLongErrorMemberName.detailValue << buildResult().withDetails(details);
}

// AFTER
// Format rule: structure.wrapBegin=40
// Format rule: structure.wrapMax=50
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

// END
// CASE	operator-wrap-chain.cpp	operator-wrap		1
// BEFORE
QByteArray lastField(const QByteArray & record)
{
    if (record.isEmpty()) {
        return QByteArray();
    }
    {
        {
            const QByteArray field = record.mid(record.indexOf(':', record.indexOf(':', record.indexOf(':', record.indexOf(':') + 1) + 1) + 1) + 1);
            return field;
        }
    }
}

// AFTER
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

// END
// CASE	operator-wrap-dot-all.cpp			1
// BEFORE
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager().configurationFromIdentifier(QLL("dummy-service-network"));
}

void chainedCalls()
{
    value = source.first().second().configurationFromIdentifier(QLL("dummy-service-network"));
    const QString shortValue = source.first().second();
}

// AFTER
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

// END
// CASE	operator-wrap-dot-any.cpp			1
// BEFORE
// Format rule: structure.target=any
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager().configurationFromIdentifier(
        QLL("dummy-service-network"));
}

// AFTER
// Format rule: structure.target=any
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager()
        .configurationFromIdentifier(QLL("dummy-service-network"));
}

// END
// CASE	operator-wrap-dot.cpp	operator-wrap		1
// BEFORE
void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager().configurationFromIdentifier(QLL("dummy-service-network"));
}

void chainedCalls()
{
    value = source.first().second().configurationFromIdentifier(QLL("dummy-service-network"));
    const QString shortValue = source.first().second();
}

// AFTER
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

// END
// CASE	operator-wrap-first-call-condition.cpp	operator-wrap		1
// BEFORE
// Format rule: structure.wrapArgsAt=never
// Format rule: structure.wrapArgsOver=argCount(0) > 20
void append()
{
    output << Packet(message.convertToInteger(), message.beginsWith(Text("sample")));
}

void appendMore()
{
    output << Packet(message.convertToInteger(), message.beginsWith(Text("sample"))) << finalItem;
}

// AFTER
// Format rule: structure.wrapArgsAt=never
// Format rule: structure.wrapArgsOver=argCount(0) > 20
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

// END
// CASE	operator-wrap-first-call.cpp	operator-wrap		1
// BEFORE
// Format rule: structure.wrapArgsAt=never
// Format rule: structure.wrapArgsOver=3
void append()
{
    output << Packet(message.convertToInteger(), message.beginsWith(Text("sample")));
}

void appendMore()
{
    output << Packet(message.convertToInteger(), message.beginsWith(Text("sample"))) << finalItem;
}

// AFTER
// Format rule: structure.wrapArgsAt=never
// Format rule: structure.wrapArgsOver=3
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

// END
// CASE	operator-wrap-logical-call.cpp			1
// BEFORE
// Format rule: structure.target=any
bool matches()
{
    return acceptsToken(next)
        || ((next == DOT || next == ARROW) && hasJoinedExpression(document, before))
        || document->token(before) == EQ;
}

// AFTER
// Format rule: structure.target=any
bool matches()
{
    return acceptsToken(next)
        || ((next == DOT || next == ARROW)
            && hasJoinedExpression(document, before))
        || document->token(before) == EQ;
}

// END
// CASE	operator-wrap-priority-assignment.cpp			1
// BEFORE
// Format rule: structure.target=any
void collect()
{
    const QStringList documentLines = QString::fromUtf8(payload.readAll()).split(QLatin1Char('\n'));
}

// AFTER
// Format rule: structure.target=any
void collect()
{
    const QStringList documentLines = QString::fromUtf8(payload.readAll())
        .split(QLatin1Char('\n'));
}

// END
// CASE	operator-wrap-priority-long-suffix.cpp			1
// BEFORE
// Format rule: structure.target=any
void collect()
{
    const QString displayName = (root.path() + separator() + component).withDetailedPresentation().withAdditionalFormattingAndLongOptionsForDisplay();
}

// AFTER
// Format rule: structure.target=any
void collect()
{
    const QString displayName = (root.path()
            + separator()
            + component
        ).withDetailedPresentation()
        .withAdditionalFormattingAndLongOptionsForDisplay();
}

// END
// CASE	operator-wrap-priority.cpp			1
// BEFORE
// Format rule: structure.target=any
void collect()
{
    const QString fullName = (root.path() + QLatin1Char('/') + pieces.at(index)).toLower();
    const QStringList documentLines = QString::fromUtf8(payload.readAll()).split(QLatin1Char('\n'));
}

// AFTER
// Format rule: structure.target=any
void collect()
{
    const QString fullName = (root.path()
            + QLatin1Char('/')
            + pieces.at(index)
        ).toLower();
    const QStringList documentLines = QString::fromUtf8(payload.readAll())
        .split(QLatin1Char('\n'));
}

// END
// CASE	param-wrap-condition.cpp	param-wrap		1
// BEFORE
// Format rule: structure.wrapArgsAt=never
// Format rule: structure.wrapArgsOver=argCount(0) > 3 && is_signature
void describe(int firstVeryLongParameter, int secondVeryLongParameter, int thirdVeryLongParameter, int fourthVeryLongParameter);

// AFTER
// Format rule: structure.wrapArgsAt=never
// Format rule: structure.wrapArgsOver=argCount(0) > 3 && is_signature
void describe(
        int firstVeryLongParameter,
        int secondVeryLongParameter,
        int thirdVeryLongParameter,
        int fourthVeryLongParameter);

// END
// CASE	short-call-adjacent-literals.cpp			1
// BEFORE
// Format rule: structure.target=any
void announce()
{
    publish(
        "note",
        Message(Text("An invented opening with several details; " "another fictional sentence adds information; " "the last sentence completes the message.")).substitute(
            value
        )
    );
}

// AFTER
// Format rule: structure.target=any
void announce()
{
    publish("note", Message(Text("An invented opening with several details; "
            "another fictional sentence adds information; "
            "the last sentence completes the message.")).substitute(value));
}

// END
// CASE	ternary-priority.cpp			1
// BEFORE
// Format rule: structure.target=any
void present()
{
    emitText("case-a", Message("A made-up description with placeholders %1 and %2").insert(theExtremelyLongComparisonValueForAlphaExtraLonger == "x" ? a : b).insert(value()));
    emitText("case-b", Message("A made-up description with placeholders %1 and %2").insert(theExtremelyLongComparisonValueForBetaWithExtraCharacters == "x" ? Text("a") : Text("b")).insert(value()));
    emitText("case-c", Message("A made-up description with placeholders %1 and %2").insert(theExtremelyLongComparisonValueForGammaWithExtraChars == "x" ? Text("an invented long positive branch for this case") : Text("an invented long negative branch for this case")).insert(value()));
    emitText("multi", Message("A made-up description with placeholders %1 and %2").insert(longPredicateWithMeaningfulName == "x" ? Text("positiveValue") : Text("negativeValue"), other == "z" ? Text("y") : Text("n"), comparisonIdentifierForThirdVeryLong == "r" ? Text("p") : Text("q")).insert(value()));
}

// AFTER
// Format rule: structure.target=any
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

// END
// CASE	ternary-wrap-cast.cpp	ternary-wrap		1
// BEFORE
int pick(int someVeryLongExpressionNameHere, int anotherQuiteLongOperand)
{
    const int value = static_cast<int>(someVeryLongExpressionNameHere) + anotherQuiteLongOperand ? yesValue : noValue;
    return value;
}

// AFTER
int pick(int someVeryLongExpressionNameHere, int anotherQuiteLongOperand)
{
    const int value =
        static_cast<int>(someVeryLongExpressionNameHere) + anotherQuiteLongOperand
        ? yesValue : noValue;
    return value;
}

// END
// CASE	ternary-wrap-head.cpp	ternary-wrap		1
// BEFORE
void pick(int type, const Item & configuration)
{
    QHash<QString, QNetworkConfigurationPrivatePointer > &table = type == QNetworkConfiguration::UserChoice ? userChoiceConfigurations : snapConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer > &other = typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice ? userChoiceConfigurations : snapConfigurations;
    SomethingShort &shortTable = typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice ? userChoiceConfigurations : snapConfigurations;
    const bool usable = configuration.isValid() || configuration.state() == Discovered ? true : fallbackCheck(configuration);
}

// AFTER
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

// END
