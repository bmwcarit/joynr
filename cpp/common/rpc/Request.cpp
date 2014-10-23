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
#include "joynr/Request.h"
#include "joynr/Util.h"
#include <QStringList>

namespace joynr
{

Request::Request() : requestReplyId(), methodName(), params(), paramDatatypes()
{
    this->requestReplyId = Util::createUuid();
}

Request::Request(const Request& other)
        : QObject(),
          requestReplyId(other.getRequestReplyId()),
          methodName(other.getMethodName()),
          params(other.getParams()),
          paramDatatypes(other.paramDatatypes)
{
}

Request& Request::operator=(const Request& other)
{
    requestReplyId = other.getRequestReplyId();
    methodName = other.getMethodName();
    params = other.getParams();
    paramDatatypes = other.paramDatatypes;
    return *this;
}

bool Request::operator==(const Request& other) const
{
    return requestReplyId == other.getRequestReplyId() && methodName == other.getMethodName() &&
           params == other.getParams() && paramDatatypes == other.paramDatatypes;
}

const QString& Request::getRequestReplyId() const
{
    return requestReplyId;
}

void Request::setRequestReplyId(const QString& requestReplyId)
{
    this->requestReplyId = requestReplyId;
}

const QString& Request::getMethodName() const
{
    return methodName;
}

void Request::setMethodName(const QString& methodName)
{
    this->methodName = methodName;
}

QList<QVariant> Request::getParams() const
{
    return params;
}

// Set the parameters - called by the QJson deserializer
void Request::setParams(const QList<QVariant>& params)
{
    this->params = params;
}

void Request::addParam(QVariant value, QString datatype)
{
    this->params.append(value);
    this->paramDatatypes.append(QVariant(datatype));
}

QList<QVariant> Request::getParamDatatypes() const
{
    return paramDatatypes;
}

// Set the parameter datatypes - called by the QJson deserializer
void Request::setParamDatatypes(const QList<QVariant>& paramDatatypes)
{
    this->paramDatatypes = paramDatatypes;
}

} // namespace joynr
