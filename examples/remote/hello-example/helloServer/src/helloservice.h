#ifndef EXAMPLE_HELLO_SERVICE_H
#define EXAMPLE_HELLO_SERVICE_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include <QtRemote/QtRemote>


// NOTE: the namespace is just proof that it's supported, but
// your code is NOT forced to have any namespace.
//
// C++ Style: lower_snake_case for namespaces,
// and UpperCamelCase for types like classes.
namespace my_lib {

class HelloService : public QObject {
    Q_REMOTE
public:
    inline explicit HelloService(QObject* parent = Q_NULLPTR)
        : QObject(parent)
    {}
    virtual ~HelloService();

public slots:
    void setName(const QString &name);
    void quit();

signals:
    void myHelloSignal(const QString &message);


    // WARNING: No such thing as `private slots` in QRemote, but
    // if really needed you could do:
private slots:
    inline QString mySecretMethod() {
        if (qobject_cast<QObjectRemote *>(this->sender())) {
            return QLL("Permission denied");
        }

        // ... Your private-logic here ...

        return QLL("My secret value");
    }

private:
    // Even if QObject already has `Q_DISABLE_COPY`, repeat it to ensure that
    // we later know this was our intention, and as a plus we get
    // better compile-error messages (if someone tries to copy).
    Q_DISABLE_COPY(HelloService)
};

} // namespace my_lib

#endif // EXAMPLE_HELLO_SERVICE_H
