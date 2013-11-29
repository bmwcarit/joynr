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
#include "cluster-controller/http-communication-manager/PingLogger.h"
#include "joynr/Util.h"
#include <QMetaEnum>

namespace joynr {

using namespace joynr_logging;


joynr_logging::Logger* PingLogger::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "PingLogger");

PingLogger::PingLogger(const QString& host, QObject* parent) :
    QObject(parent),
    ping(this)
{
    connect(
            &ping, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(error(QProcess::ProcessError))
    );
    connect(
            &ping, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(finished(int,QProcess::ExitStatus))
    );
    connect(
            &ping, SIGNAL(readyReadStandardError()),
            this, SLOT(readyReadStandardError())
    );
    connect(
            &ping, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readyReadStandardOutput())
    );
    connect(
            &ping, SIGNAL(started()),
            this, SLOT(started())
    );
    connect(
            &ping, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(stateChanged(QProcess::ProcessState))
    );
    QString cmd = "ping";
    QStringList args;
#ifdef Q_OS_WIN32
    args << "-t";
#endif

    args << host;

    ping.start(cmd, args);
}

PingLogger::~PingLogger() {
    LOG_DEBUG(logger, "starting ~PingLogger");
    ping.kill();
    ping.waitForFinished();
    LOG_DEBUG(logger, "finished ~PingLogger");
}

void PingLogger::error(QProcess::ProcessError error) {
    // for some reasons QMetaObject for QProcess is not working
    // so the Util::convertEnumValueToString will not work
//    LOG_DEBUG(
//            logger,
//            QString("error while executing ping: ")
//                +Util::convertEnumValueToString<QProcess>("ProcessError", error)
//    );

    QString errorStr;
    switch(error) {
    case QProcess::FailedToStart:
        errorStr = "FailedToStart";
        break;
    case QProcess::Crashed:
        errorStr = "Crashed";
        break;
    case QProcess::Timedout:
        errorStr = "Timedout";
        break;
    case QProcess::WriteError:
        errorStr = "WriteError";
        break;
    case QProcess::ReadError:
        errorStr = "ReadError";
        break;
    case QProcess::UnknownError :
        errorStr = "UnknownError ";
        break;
    default:
        errorStr = "unknown";
    }

    LOG_DEBUG(logger, QString("error while executing ping: ")+errorStr);
}

void PingLogger::finished(int exitCode, QProcess::ExitStatus exitStatus) {
// for some reasons QMetaObject for QProcess is not working
// so the Util::convertEnumValueToString will not work
//    LOG_DEBUG(
//            logger,
//            QString("ping finished; exit code: ")+exitCode
//                +QString("; exit status: ")
//                +Util::convertEnumValueToString<QProcess>("ExitStatus", exitStatus)
//    );

    QString exitStatusStr;
    switch(exitStatus) {
    case QProcess::NormalExit:
        exitStatusStr = "NormalExit";
        break;
    case QProcess::CrashExit :
        exitStatusStr = "CrashExit ";
        break;
    default:
        exitStatusStr = "unknown";
    }

    LOG_DEBUG(
            logger,
            QString("ping finished; exit code: ")+QString::number(exitCode)
                +QString("; exit status: ")+ exitStatusStr
    );

}

void PingLogger::readyReadStandardError() {
//    LOG_TRACE(logger, QString("data available on standard error"));
    ping.setReadChannel(QProcess::StandardError);
    readAndLogAvailableInput();
}

void PingLogger::readAndLogAvailableInput() {
    LOG_DEBUG(logger, QString(ping.readAll()));
}

void PingLogger::readyReadStandardOutput() {
//    LOG_TRACE(logger, QString("data available on standard output"));
    ping.setReadChannel(QProcess::StandardOutput);
    readAndLogAvailableInput();
}

void PingLogger::started() {
    LOG_DEBUG(logger, QString("ping process started"));
}

void PingLogger::stateChanged(QProcess::ProcessState newState) {
// for some reasons QMetaObject for QProcess is not working
// so the Util::convertEnumValueToString will not work
//    LOG_DEBUG(
//            logger,
//            QString("ping process state changed to ")
//                +Util::convertEnumValueToString<QProcess>("ProcessState", newState)
//    );

    QString stateStr;
    switch(newState) {
    case QProcess::NotRunning:
        stateStr = "NotRunning";
        break;
    case QProcess::Starting:
        stateStr = "Starting";
        break;
    case QProcess::Running:
        stateStr = "Running";
        break;
    default:
        stateStr = "unknown";
    }

    LOG_DEBUG(logger, QString("ping process state changed to ")+stateStr);

}



} // namespace joynr
