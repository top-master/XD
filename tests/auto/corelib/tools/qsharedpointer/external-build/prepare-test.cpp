
#define QT_SHAREDPOINTER_TRACK_POINTERS
#define QT_DEBUG
#include <QtCore/qsharedpointer.h>
#include <QtCore/qcoreapplication.h>


#if 1
struct Data
{
public:
    static int destructorCounter;
    static int generationCounter;
    int i;
    int generation;

    Data() : generation(++generationCounter)
    { }

    virtual ~Data()
    {
        if (generation <= 0)
            qFatal("tst_qsharedpointer: Double deletion!");
        generation = 0;
        ++destructorCounter;
    }

    void doDelete()
    {
        delete this;
    }

    bool alsoDelete()
    {
        doDelete();
        return true;
    }

    virtual void virtualDelete()
    {
        delete this;
    }

    virtual int classLevel() { return 1; }
};
int Data::generationCounter = 0;
int Data::destructorCounter = 0;
#else
struct Data { int i; };
#endif

enum {
    DataIsComplex = QTypeInfo<Data>::isComplex
};

struct DerivedData: public Data { int j; };

class ForwardDeclared;
