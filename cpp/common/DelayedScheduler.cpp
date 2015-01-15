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
#include "joynr/DelayedScheduler.h"

#include <QtCore>
#include <QSignalMapper>
#include <QCoreApplication>
#include "joynr/joynrlogging.h"
#include "QMutableHashIterator"
#include "joynr/Util.h"

namespace joynr
{

using namespace joynr_logging;
Logger* DelayedScheduler::logger = Logging::getInstance()->getLogger("MSG", "DelayedScheduler");

DelayedScheduler::DelayedScheduler(const QString& eventThreadName, int delay_ms)
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

DelayedScheduler::~DelayedScheduler()
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

quint32 DelayedScheduler::INVALID_RUNNABLE_HANDLE()
{
    static const quint32 value(0);
    return value;
}

void DelayedScheduler::shutdown()
{
    // LOG_TRACE(logger, "shutdown: entering...");
    QMutexLocker locker(&mutex);
    // LOG_TRACE(logger, "shutdown: locked mutex...");
    stoppingScheduler = true;
    // LOG_TRACE(logger, "shutdown: stopping running timers");

    QMutableHashIterator<QTimer*, quint32> iterator(timers);

    while (iterator.hasNext()) {
        iterator.next();
        QTimer* timer = iterator.key();
        QRunnable* runnable = runnables.take(iterator.value());

        assert(runnable != NULL);
        if (runnable->autoDelete()) {
            delete runnable;
        }

        assert(timer != NULL);
        QMetaObject::invokeMethod(timer, "stop", Qt::QueuedConnection);
        iterator.remove();
        timer->deleteLater();
    }

    assert(runnables.size() == 0);
}

void DelayedScheduler::unschedule(quint32& runnableHandle)
{
    QMutexLocker locker(&mutex);
    if (runnables.contains(runnableHandle)) {
        QRunnable* runnable = runnables.take(runnableHandle);
        QMutableHashIterator<QTimer*, quint32> iterator(timers);
        if (iterator.findNext(runnableHandle)) {
            QTimer* timer = iterator.key();
            timers.remove(timer);
            timer->deleteLater();
            QMetaObject::invokeMethod(timer, "stop", Qt::QueuedConnection);
        }

        if (runnable->autoDelete()) {
            delete runnable;
        }
    } else {
        LOG_TRACE(logger,
                  "unschedule did not succeed. Provided runnableHandle " + QString(runnableHandle) +
                          " could not be found by the scheduler");
    }
}

quint32 DelayedScheduler::schedule(QRunnable* runnable, int delay_ms /* = -1*/)
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

void DelayedScheduler::scheduleUsingTimer(void* runnable, quint32 runnableHandle, int delay_ms)
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
    runnables.insert(runnableHandle, (QRunnable*)runnable);
    timers.insert(timer, runnableHandle);

    // Connect the timer to call run()
    connect(timer, SIGNAL(timeout()), this, SLOT(run()));

    // Start the timer
    timer->start(delay_ms);

    LOG_TRACE(logger, "scheduleUsingTimer: leaving...");
}

void DelayedScheduler::run()
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

        if (timers.contains(timer)) {
            runnable = runnables.take(timers.take(timer));
        }
    }

    if (runnable != NULL) {
        // executeRunnable does not require a mutex lock
        executeRunnable(runnable);
    }
}

ThreadPoolDelayedScheduler::ThreadPoolDelayedScheduler(QThreadPool& threadPool,
                                                       const QString& eventThreadName,
                                                       int delay_ms)
        : DelayedScheduler(eventThreadName, delay_ms), threadPool(threadPool)
{
    threadPool.setObjectName(eventThreadName + QString("-ThreadPool"));
}

void ThreadPoolDelayedScheduler::executeRunnable(QRunnable* runnable)
{
    threadPool.start(runnable);
}

Logger* SingleThreadedDelayedScheduler::logger =
        Logging::getInstance()->getLogger("MSG", "SingleThreadedDelayedScheduler");

SingleThreadedDelayedScheduler::SingleThreadedDelayedScheduler(const QString& eventThreadName,
                                                               int delay_ms)
        : DelayedScheduler(eventThreadName, delay_ms)
{
}

SingleThreadedDelayedScheduler::~SingleThreadedDelayedScheduler()
{
    shutdown();
}

void SingleThreadedDelayedScheduler::executeRunnable(QRunnable* runnable)
{
    LOG_TRACE(logger, "executeRunnable: entering ...");
    runnable->run();
    if (runnable->autoDelete()) {
        LOG_TRACE(logger, "executeRunnable: deleting runnable.");
        delete runnable;
    }
    LOG_TRACE(logger, "executeRunnable: leaving...");
}

ThreadPoolDelayedScheduler::~ThreadPoolDelayedScheduler()
{
    shutdown();
    threadPool.waitForDone();
}

DelayedScheduler::EventThread::EventThread()
{
}

DelayedScheduler::EventThread::~EventThread()
{
    // LOG_TRACE(logger, "EventThread destructor ...");
}

void DelayedScheduler::EventThread::run()
{
    // LOG_TRACE(logger, "EventThread.run: entering...");
    exec();
    // LOG_TRACE(logger, "EventThread.run: event loop stopped. processing remaining events...");
    QCoreApplication::processEvents();
    // LOG_TRACE(logger, "EventThread.run: leaving... event thread stopped.");
}

} // namespace joynr
