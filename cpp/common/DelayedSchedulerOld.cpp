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
#include "joynr/DelayedSchedulerOld.h"

#include <QtCore>
#include <QSignalMapper>
#include <QCoreApplication>
#include "joynr/joynrlogging.h"
#include "QMutableHashIterator"
#include "joynr/Util.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

using namespace joynr_logging;
Logger* DelayedSchedulerOld::logger = Logging::getInstance()->getLogger("MSG", "DelayedScheduler");

DelayedSchedulerOld::DelayedSchedulerOld(const QString& eventThreadName, int delay_ms)
        : delay_ms(delay_ms),
          runnableHandle(INVALID_RUNNABLE_HANDLE()),
          eventThread(),
          runnables(),
          timers(),
          mutex(QMutex::Recursive),
          stoppingScheduler(false)
{
    eventThread.setObjectName(eventThreadName);
    moveToThread(&eventThread); // So that timers can be created as children of this
    eventThread.start();
}

DelayedSchedulerOld::~DelayedSchedulerOld()
{
    // LOG_TRACE(logger, "destructor: entering...");

    if (QThread::currentThread() != &eventThread) {
        // LOG_TRACE(logger, "destructor: stopping event thread");
        eventThread.quit();
        // LOG_TRACE(logger, "destructor: waiting for termination of event thread...");
        eventThread.wait();
        // LOG_TRACE(logger, "destructor: leaving... event thread stopped. All runnables
        // finished.");
    }
}

quint32 DelayedSchedulerOld::INVALID_RUNNABLE_HANDLE()
{
    static const quint32 value(0);
    return value;
}

void DelayedSchedulerOld::shutdown()
{
    // LOG_TRACE(logger, "shutdown: entering...");
    QMutexLocker locker(&mutex);
    // LOG_TRACE(logger, "shutdown: locked mutex...");
    stoppingScheduler = true;
    // LOG_TRACE(logger, "shutdown: stopping running timers");

    auto timersIterator = timers.begin();
    while (timersIterator != timers.end()) {
        QTimer* timer = timersIterator->first;
        QRunnable* runnable;

        auto runnablesIterator = runnables.find(timersIterator->second);
        if (runnablesIterator != runnables.end()) {
            runnable = runnablesIterator->second;
            runnables.erase(runnablesIterator);
        }

        assert(runnable != NULL);
        if (runnable->autoDelete()) {
            delete runnable;
        }

        assert(timer != NULL);
        QMetaObject::invokeMethod(timer, "stop", Qt::QueuedConnection);

        timersIterator = timers.erase(timersIterator);

        timer->deleteLater();
    }

    assert(runnables.size() == 0);
}

void DelayedSchedulerOld::unschedule(quint32& runnableHandle)
{
    QMutexLocker locker(&mutex);
    auto runnableIterator = runnables.find(runnableHandle);
    if (runnableIterator != runnables.end()) {
        QRunnable* runnable = runnableIterator->second;
        runnables.erase(runnableHandle);

        auto timersIterator = timers.begin();
        while (timersIterator != timers.end()) {
            if (timersIterator->second == runnableHandle) {
                QTimer* timer = timersIterator->first;
                timersIterator = timers.erase(timersIterator);
                QMetaObject::invokeMethod(timer, "stop", Qt::QueuedConnection);
                timer->deleteLater();
            } else {
                ++timersIterator;
            }
        }

        if (runnable->autoDelete()) {
            delete runnable;
        }
        LOG_TRACE(logger, QString("runnable with handle %1 unscheduled").arg(runnableHandle));
    } else {
        LOG_TRACE(logger,
                  "unschedule did not succeed. Provided runnableHandle " + QString(runnableHandle) +
                          " could not be found by the scheduler");
    }
}

quint32 DelayedSchedulerOld::schedule(QRunnable* runnable, int delay_ms /* = -1*/)
{
    LOG_DEBUG(logger, QString("schedule: entering with delay %1").arg(delay_ms));
    if (delay_ms == -1) {
        delay_ms = this->delay_ms;
    }

    QMutexLocker locker(&mutex);
    runnableHandle++;
    if (stoppingScheduler) {
        delete runnable;
        return runnableHandle;
    }
    /* Scheduling should be done in the eventThread
      to ensure that the timer is created in that thread. */
    QMetaObject::invokeMethod(this,
                              "scheduleUsingTimer",
                              Qt::QueuedConnection,
                              Q_ARG(void*, runnable),
                              Q_ARG(quint32, runnableHandle),
                              Q_ARG(int, delay_ms));
    return runnableHandle;
}

void DelayedSchedulerOld::scheduleUsingTimer(void* runnable, quint32 runnableHandle, int delay_ms)
{
    LOG_TRACE(logger, QString("scheduleUsingTimer: entering with delay %1").arg(delay_ms));
    QMutexLocker locker(&mutex);

    if (stoppingScheduler) {
        delete (QRunnable*)runnable;
        return;
    }

    // Create a timer
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);

    // Make note of the runnable that will be executed
    runnables.insert({runnableHandle, (QRunnable*)runnable});
    timers.insert({timer, runnableHandle});

    // Connect the timer to call run()
    connect(timer, SIGNAL(timeout()), this, SLOT(run()));

    // Start the timer
    timer->start(delay_ms);

    LOG_TRACE(logger, "scheduleUsingTimer: leaving...");
}

void DelayedSchedulerOld::run()
{
    QRunnable* runnable = NULL;
    {
        QMutexLocker locker(&mutex);
        // LOG_TRACE(logger, "run: locked mutex.");
        if (stoppingScheduler) {
            // LOG_TRACE(logger, "run: scheduler stopped. Aborting...");
            return;
        }
        QTimer* timer = reinterpret_cast<QTimer*>(sender());
        timer->deleteLater();

        auto timersIterator = timers.find(timer);
        if (timersIterator != timers.end()) {
            timers.erase(timersIterator);

            auto runnablesIterator = runnables.find(timersIterator->second);
            if (runnablesIterator != runnables.end()) {
                runnable = runnablesIterator->second;
                runnables.erase(runnablesIterator);
            }
        }
    }

    if (runnable != NULL) {
        // executeRunnable does not require a mutex lock
        executeRunnable(runnable);
    }
}

class ThreadPoolDelayedSchedulerOld::ThreadPoolRunnable : public QRunnable
{
public:
    ThreadPoolRunnable(QRunnable* runnable, ThreadPoolDelayedSchedulerOld& scheduler);
    virtual ~ThreadPoolRunnable();
    virtual void run();

private:
    QRunnable* runnable;
    ThreadPoolDelayedSchedulerOld& scheduler;

    DISALLOW_COPY_AND_ASSIGN(ThreadPoolRunnable);
};

ThreadPoolDelayedSchedulerOld::ThreadPoolRunnable::ThreadPoolRunnable(
        QRunnable* runnable,
        ThreadPoolDelayedSchedulerOld& scheduler)
        : QRunnable(), runnable(runnable), scheduler(scheduler)
{
    // auto-deletion is enabled by default
}

ThreadPoolDelayedSchedulerOld::ThreadPoolRunnable::~ThreadPoolRunnable()
{
    if (runnable->autoDelete()) {
        delete runnable;
    }
}

void ThreadPoolDelayedSchedulerOld::ThreadPoolRunnable::run()
{
    scheduler.reportRunnableStarted();
    runnable->run();
}

Logger* ThreadPoolDelayedSchedulerOld::logger =
        Logging::getInstance()->getLogger("MSG", "ThreadPoolDelayedScheduler");

ThreadPoolDelayedSchedulerOld::ThreadPoolDelayedSchedulerOld(QThreadPool& threadPool,
                                                             const QString& eventThreadName,
                                                             int delay_ms)
        : DelayedSchedulerOld(eventThreadName, delay_ms),
          threadPool(threadPool),
          waitingRunnablesCount(0),
          waitingRunnablesCountMutex()
{
    threadPool.setObjectName(eventThreadName + QString("-ThreadPool"));
}

void ThreadPoolDelayedSchedulerOld::executeRunnable(QRunnable* runnable)
{
    {
        QMutexLocker locker(&waitingRunnablesCountMutex);
        waitingRunnablesCount++;
    }
    threadPool.start(new ThreadPoolRunnable(runnable, *this));
    LOG_TRACE(logger,
              QString("scheduler waiting runnables (active threads/max threads): %1 (%2/%3)")
                      .arg(waitingRunnablesCount)
                      .arg(threadPool.activeThreadCount())
                      .arg(threadPool.maxThreadCount()));
}

Logger* SingleThreadedDelayedSchedulerOld::logger =
        Logging::getInstance()->getLogger("MSG", "SingleThreadedDelayedScheduler");

SingleThreadedDelayedSchedulerOld::SingleThreadedDelayedSchedulerOld(const QString& eventThreadName,
                                                                     int delay_ms)
        : DelayedSchedulerOld(eventThreadName, delay_ms)
{
}

SingleThreadedDelayedSchedulerOld::~SingleThreadedDelayedSchedulerOld()
{
    shutdown();
}

void SingleThreadedDelayedSchedulerOld::executeRunnable(QRunnable* runnable)
{
    LOG_TRACE(logger, "executeRunnable: entering ...");
    runnable->run();
    if (runnable->autoDelete()) {
        LOG_TRACE(logger, "executeRunnable: deleting runnable.");
        delete runnable;
    }
    LOG_TRACE(logger, "executeRunnable: leaving...");
}

ThreadPoolDelayedSchedulerOld::~ThreadPoolDelayedSchedulerOld()
{
    shutdown();
    threadPool.waitForDone();
}

void ThreadPoolDelayedSchedulerOld::reportRunnableStarted()
{
    {
        QMutexLocker locker(&waitingRunnablesCountMutex);
        waitingRunnablesCount--;
    }
    LOG_TRACE(logger,
              QString("scheduler waiting runnables (active threads/max threads): %1 (%2/%3)")
                      .arg(waitingRunnablesCount)
                      .arg(threadPool.activeThreadCount())
                      .arg(threadPool.maxThreadCount()));
}

DelayedSchedulerOld::EventThread::EventThread()
{
}

DelayedSchedulerOld::EventThread::~EventThread()
{
    // LOG_TRACE(logger, "EventThread destructor ...");
}

void DelayedSchedulerOld::EventThread::run()
{
    // LOG_TRACE(logger, "EventThread.run: entering...");
    exec();
    // LOG_TRACE(logger, "EventThread.run: event loop stopped. processing remaining events...");
    QCoreApplication::processEvents();
    // LOG_TRACE(logger, "EventThread.run: leaving... event thread stopped.");
}

} // namespace joynr
