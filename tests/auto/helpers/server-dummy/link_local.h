#ifndef LINK_LOCAL_H
#define LINK_LOCAL_H

#include "./dynamic_config.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>

#if defined(Q_OS_UNIX)
#  include <sys/wait.h>
#  include <unistd.h>
#endif

/// Lets the loopback carry an IPv4 link-local address while server-config.ini
/// [Network] linkLocalIPv4 is on, for a test that needs such an address on an
/// interface of its own host (`tst_QUdpSocket::linkLocalIPv4`).
///
/// Only an elevated process may change an interface; hence the switch does
/// nothing otherwise. Qt has no interface to change one, hence it runs the
/// platform's own tool: `ip` on Linux, `ifconfig` on the other Unix systems,
/// `netsh` on Windows. The address goes again once the switch turns off, which
/// a test's `TestServerEditUndoer` does when its row ends, and, on Unix, when
/// the server ends by SIGTERM or SIGINT (see removeFromSignal()).
class LinkLocalAddress
{
public:
    /// Names the address the loopback carries; a /16 of 169.254.0.0.
    static const char *address() { return "169.254.255.1"; }

    /// Adds or removes the address, as @p wanted asks; does nothing once the
    /// loopback already is that way, or in a process that is not elevated.
    static void apply(bool wanted)
    {
        if (wanted == isAdded() || !QCoreApplication::isElevated()) {
            return;
        }
        const QStringList command = wanted ? addCommand() : removeCommand();
        if (command.isEmpty()) {
            return;
        }
        const QString program = QStandardPaths::findExecutable(command.first());
        if (program.isEmpty()) {
            return;
        }
        if (QProcess::execute(program, command.mid(1)) != 0) {
            return;
        }
        isAdded() = wanted;
#if defined(Q_OS_UNIX)
        prepareRemoval(program, removeCommand());
#endif
    }

#if defined(Q_OS_UNIX)
    /// Removes the address from a signal handler, through async-signal-safe
    /// calls only: `fork()` and `execve()` of the command that apply() kept.
    static void removeFromSignal()
    {
        if (!isAdded() || !removalArgv()[0]) {
            return;
        }
        const pid_t child = ::fork();
        if (child == 0) {
            char *const environment[] = { Q_NULLPTR };
            ::execve(removalArgv()[0], removalArgv(), environment);
            ::_exit(127);
        }
        if (child > 0) {
            int status = 0;
            ::waitpid(child, &status, 0);
        }
    }
#endif

private:
    /// Tells whether this process added the address and did not remove it yet.
    static bool &isAdded()
    {
        static bool added = false;
        return added;
    }

    static QStringList addCommand()
    {
#if defined(Q_OS_LINUX)
        return QStringList() << QLL("ip") << QLL("addr") << QLL("add")
                             << (QLatin1String(address()) + QLL("/16")) << QLL("dev") << QLL("lo");
#elif defined(Q_OS_UNIX)
        return QStringList() << QLL("ifconfig") << QLL("lo0") << QLL("alias")
                             << QLatin1String(address()) << QLL("netmask") << QLL("255.255.0.0");
#elif defined(Q_OS_WIN)
        return QStringList() << QLL("netsh") << QLL("interface") << QLL("ipv4") << QLL("add")
                             << QLL("address") << QLL("name=Loopback Pseudo-Interface 1")
                             << (QLL("address=") + QLatin1String(address()))
                             << QLL("mask=255.255.0.0");
#else
        return QStringList();
#endif
    }

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

#if defined(Q_OS_UNIX)
    /// Gives removeFromSignal() the removal command in the form `execve()` takes,
    /// built while no signal can interrupt; empty until apply() added the address.
    static char **removalArgv()
    {
        static char *argv[8] = { Q_NULLPTR };
        return argv;
    }

    static void prepareRemoval(const QString &program, const QStringList &command)
    {
        static QByteArray storage[8];
        char **argv = removalArgv();
        const int count = qMin(command.size(), 7);
        storage[0] = program.toLocal8Bit();
        argv[0] = storage[0].data();
        for (int i = 1; i < count; ++i) {
            storage[i] = command.at(i).toLocal8Bit();
            argv[i] = storage[i].data();
        }
        argv[count] = Q_NULLPTR;
    }
#endif
};

#endif // LINK_LOCAL_H
