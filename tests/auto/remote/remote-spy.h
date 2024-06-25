
#ifndef QT_REMOTE_SPY_H
#define QT_REMOTE_SPY_H

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QRemoteUser>


class UserSpy {
public:
    inline UserSpy(QRemoteUser &user)
        : spyNewConnection(&user, &QRemoteUser::newConnection)
    {
        qExpect(spyNewConnection.isValid())->toBeTruthy();
    }

    inline QSharedPointer<QObjectRemote> assertNewConnection(int index) {
        qExpect(spyNewConnection.count())->toBeGreaterOrEqual(index + 1)
                ->withTraceSkip(":assert");
        // With right argument count.
        qExpect(spyNewConnection[index].count())->toEqual(1);
        // With right argument type.
        QVariant arg = spyNewConnection[index][0];
        qExpect(arg)->Not->toBeNull();
        int refTypeId = qMetaTypeId<QRef<QObject>>();
        qExpect(arg.type())->toEqual(refTypeId);
        // With right argument value.
        QSharedPointer<QObjectRemote> controller = arg.value<QRef<QObjectRemote> >();
        qExpect(controller.data())->Not->toBeNull();

        return controller;
    }

protected:
    QSignalSpy spyNewConnection;
};



#endif // QT_REMOTE_SPY_H
