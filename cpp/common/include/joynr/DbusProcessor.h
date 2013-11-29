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
#ifndef DBUSPROCESSOR_H
#define DBUSPROCESSOR_H
#include "joynr/PrivateCopyAssign.h"

#include <QFuture>
#include <QString>
#include <QSemaphore>

namespace joynr {

namespace joynr_logging { class Logger; }

class DbusProcessor {
public:
    DbusProcessor(const QString& dbusBusName);
    virtual ~DbusProcessor();

    void startDbusMainLoop();
    void stopDbusMainLoop();
    void requestName();

private:
    DISALLOW_COPY_AND_ASSIGN(DbusProcessor);
    static ::joynr_logging::Logger* logger;

    void executeDbusMainLoop();

    bool processDbus;
    QSemaphore dbusMainLoopStarted;
    QString dbusBusName;
    QFuture<void> dbusFuture;
};

} // namespace joynr
#endif //DBUSPROCESSOR_H
