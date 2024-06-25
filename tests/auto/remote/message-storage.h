#ifndef QMESSAGESTORAGE_H
#define QMESSAGESTORAGE_H

#include <QtCore/qstring.h>
#include <QtCore/qlogging.h>
#include <QtCore/qlinkedlist.h>

class MessageEntry {
public:
    QtMsgType type;
    QMessageLogContext context;
    const QString message;

    Q_ALWAYS_INLINE MessageEntry(
            QtMsgType typeArg, const QMessageLogContext &contextArg,
            const QString &messageArg)
        : type(typeArg)
        , context(contextArg.clone())
        , message(messageArg)
    {
    }

    inline MessageEntry(const MessageEntry &other)
        : type(other.type)
        , context(other.context.clone())
        , message(other.message)
    {
    }
};

typedef QLinkedList<MessageEntry> MessageEntryList;

class MessageStorage {
public:
    inline MessageStorage() : m_oldHandler(Q_NULLPTR) {}

    inline QtMessageHandler registerHandler() {
        m_oldHandler = qInstallMessageHandler(&MessageStorage::msgStore);
        return m_oldHandler;
    }

    static MessageStorage &get();

    void flush(const QtMessageHandler &handler = Q_NULLPTR);

private:
    static void msgStore(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    QtMessageHandler m_oldHandler;

public:
    MessageEntryList list;
};

#endif // QMESSAGESTORAGE_H
