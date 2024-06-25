
#include "./message-storage.h"

#include <QtCore/qthreadstorage.h>


Q_GLOBAL_STATIC(QThreadStorage<MessageStorage>, globalThreadStorage)


MessageStorage &MessageStorage::get() {
    QThreadStorage<MessageStorage> *storage = globalThreadStorage();
    Q_ASSERT(storage);
    MessageStorage &store = storage->localData();
    return store;
}

void MessageStorage::msgStore(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    MessageStorage &store = MessageStorage::get();
    store.list.append(MessageEntry(type, context, msg));
}

/**
 * Passes all stored messages to given handler, and clears own storage.
 */
void MessageStorage::flush(const QtMessageHandler &handler) {
    Q_ASSERT(handler);
    QtMessageHandler handlerResolved = handler != Q_NULLPTR
            ? handler : this->m_oldHandler;
    MessageEntryList &list = this->list;
    MessageEntryList::const_iterator it = list.cbegin();
    MessageEntryList::const_iterator end = list.cend();
    for (; it != end; ++it) {
        const MessageEntry &entry = *it;
        handlerResolved(entry.type, entry.context, entry.message);
    }
    list.clear();
}
