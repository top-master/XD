// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-thread-runner.h"

#ifndef QT_NO_THREAD

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>

/*!
    @class FormatThreadQueue
    @internal

    Hands the files of one FormatThreadRunner::run() call to its threads one
    at a time, and gathers their results in the order of the files.
*/
class FormatThreadQueue
{
public:
    inline explicit FormatThreadQueue(const QStringList & files)
        : files(files), next(0)
    {
        for (int i = 0; i < files.size(); ++i) {
            results.append(FormatFileResult());
        }
    }

    /// Guards `next` and `results`, which every thread of the run changes.
    QMutex mutex;
    /// Lists the files of the run, in the order their results keep.
    QStringList files;
    /// Points at the first file no thread took yet.
    int next;
    /// Collects each file's result at the file's index.
    QList<FormatFileResult > results;
};

/*!
    @class FormatThread
    @internal

    Formats files from a FormatThreadQueue until none is left.
*/
class FormatThread : public QThread
{
    typedef QThread super;
public:
    inline FormatThread(const FormatRunner *runner, FormatThreadQueue *queue)
        : m_runner(runner), m_queue(queue)
    {
    }

protected:
    void run() Q_DECL_OVERRIDE;

private:
    /// Formats each file the thread takes.
    const FormatRunner *m_runner;
    /// Hands out the files, and takes back their results.
    FormatThreadQueue *m_queue;
};

/*!
    Takes the next file of the queue, formats it, and stores its result,
    until the queue is empty.
*/
void FormatThread::run()
{
    for (;;) {
        int index = 0;
        {
            QMutexLocker locker(&this->m_queue->mutex);
            if (this->m_queue->next >= this->m_queue->files.size()) {
                return;
            }
            index = this->m_queue->next++;
        }
        const FormatFileResult result = qMove(this->m_runner->formatFile(this->m_queue->files.at(index)));
        QMutexLocker locker(&this->m_queue->mutex);
        this->m_queue->results[index] = result;
    }
}

/*!
    @class FormatThreadRunner
    @internal

    Formats the files of a run in up to jobCount() threads at once.
*/

/*!
    Builds the runner that formats with @a formatter as @a options say.
*/
FormatThreadRunner::FormatThreadRunner(const Formatter *formatter, const FormatOptions & options)
    : super(formatter, options)
{
}

/*!
    Formats @a files in threads, and returns their results in the order of
    @a files.
*/
QList<FormatFileResult > FormatThreadRunner::run(const QStringList & files)
{
    const int jobs = this->jobCount(files.size());
    if (jobs <= 1) {
        return super::run(files);
    }

    FormatThreadQueue queue(files);
    QList<FormatThread * > threads;
    for (int i = 0; i < jobs; ++i) {
        FormatThread *thread = new FormatThread(this, &queue);
        threads.append(thread);
        thread->start();
    }
    for (int i = 0; i < threads.size(); ++i) {
        threads.at(i)->wait();
    }
    qDeleteAll(threads);
    return queue.results;
}

#endif // QT_NO_THREAD
