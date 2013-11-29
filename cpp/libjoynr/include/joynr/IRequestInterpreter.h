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
#ifndef IREQUESTINTERPRETER_H
#define IREQUESTINTERPRETER_H

#include <QVariant>
#include <QMap>
#include <QSharedPointer>

namespace joynr {

class RequestCaller;

/**
  * Common interface for all \class <Intf>RequestInterpreter.
  */
class IRequestInterpreter {
public:
    virtual ~IRequestInterpreter(){}

    /**
      * Executes method \param methodName with parameters \param methodParams
      * on the \param requestCaller object.
      */
    virtual QVariant execute(QSharedPointer<RequestCaller> requestCaller,
                             const QString& methodName,
                             const QList<QVariant>& paramValues,
                             const QList<QVariant>& paramTypes)=0;
};


} // namespace joynr
#endif //IREQUESTINTERPRETER_H
