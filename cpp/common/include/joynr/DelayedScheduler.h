/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef DELAYED_SCHEDULER_H_
#define DELAYED_SCHEDULER_H_

#include "joynr/JoynrCommonExport.h"

#include <QThreadPool>
#include <QRunnable>
#include <unordered_map>
#include <QMutex>
#include <QSemaphore>
#include <QEventLoop>
#include <QTimer>

namespace joynr
{

namespace joynr_logging
{
class Logger;
}

/**
  * An abstract base class to schedule QRunnables to be executed at some time in the future.
  *
  * Internally uses a newly started thread with a QT event queue.
  * Because the thread is stopped in the destructor, destruction of this object can take some time.
  */
class JOYNRCOMMON_EXPORT DelayedScheduler : public QObject
{
    Q_OBJECT

public:
    DelayedScheduler(const QString& eventThreadName, int delay_ms = 0);
    virtual ~DelayedScheduler();

    static quint32 INVALID_RUNNABLE_HANDLE();

    /**
      * Schedules a runnable to be executed after delay_ms has passed.
      * If no delay is supplied the default delay specified in the constructor is used.
      */
    virtual quint32 schedule(QRunnable* runnable, int delay_ms = -1);
    virtual void unschedule(quint32& runnableHandle);

protected:
    virtual void executeRunnable(QRunnable* runnable) = 0;
    void shutdown();

private slots:
    void scheduleUsingTimer(void* runnable, quint32 runnableHandle, int delay_ms);
    void run();

private:
    class EventThread : public QThread
    {
    public:
        EventThread();
        virtual ~EventThread();
        void run();
    };

    int delay_ms;
    quint32 runnableHandle;

    EventThread eventThread;
    std::unordered_map<quint32, QRunnable*> runnables;
    std::unordered_map<QTimer*, quint32> timers;
    QMutex mutex;
    bool stoppingScheduler;
    static joynr_logging::Logger* logger;
};

/**
  * An implementation of the DelayedScheduler that uses the ThreadPool passed in the constructor to
 * execute the runnables.
  */
class JOYNRCOMMON_EXPORT ThreadPoolDelayedScheduler : public DelayedScheduler
{
    Q_OBJECT

public:
    ThreadPoolDelayedScheduler(QThreadPool& threadPool,
                               const QString& eventThreadName,
                               int delay_ms = 0);
    virtual ~ThreadPoolDelayedScheduler();

    void reportRunnableStarted();

protected:
    void executeRunnable(QRunnable* runnable);

private:
    static joynr_logging::Logger* logger;
    QThreadPool& threadPool;
    int waitingRunnablesCount;
    QMutex waitingRunnablesCountMutex;

    class ThreadPoolRunnable;
};

/**
  * An implementation of the DelayedScheduler that uses the event thread to execute the runnables.
  * This implementation should not be used for runnables that take substantial time to complete.
  */
class JOYNRCOMMON_EXPORT SingleThreadedDelayedScheduler : public DelayedScheduler
{
    Q_OBJECT

public:
    SingleThreadedDelayedScheduler(const QString& eventThreadName, int delay_ms = 0);
    virtual ~SingleThreadedDelayedScheduler();

protected:
    void executeRunnable(QRunnable* runnable);

private:
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // DELAYED_SCHEDULER_H_
