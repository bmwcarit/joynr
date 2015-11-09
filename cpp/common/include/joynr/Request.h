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
#ifndef REQUEST_H
#define REQUEST_H

#include "joynr/JoynrCommonExport.h"

#include <QObject>
#include <QVariant>

#include <memory>
#include <string>
#include <vector>

namespace joynr
{

class JOYNRCOMMON_EXPORT Request : public QObject
{
    Q_OBJECT

    Q_PROPERTY(std::string requestReplyId READ getRequestReplyId WRITE setRequestReplyId)
    Q_PROPERTY(std::string methodName READ getMethodName WRITE setMethodName)
    Q_PROPERTY(std::vector<QVariant> params READ getParams WRITE setParams)
    Q_PROPERTY(std::vector<std::string> paramDatatypes READ getParamDatatypes WRITE
                       setParamDatatypes)

public:
    Request();

    Request(const Request& other);
    Request& operator=(const Request& other);
    bool operator==(const Request& other) const;

    const std::string& getRequestReplyId() const;
    void setRequestReplyId(const std::string& requestReplyId);

    const std::string& getMethodName() const;
    void setMethodName(const std::string& methodName);

    std::vector<QVariant> getParams() const;
    void setParams(const std::vector<QVariant>& params);

    void addParam(QVariant value, std::string datatype);

    std::vector<std::string> getParamDatatypes() const;
    void setParamDatatypes(const std::vector<std::string>& paramDatatypes);

private:
    static QVariant parameterType(const QVariant& param);

    std::string requestReplyId;
    std::string methodName;
    std::vector<QVariant> params;
    std::vector<std::string> paramDatatypes;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::Request)
Q_DECLARE_METATYPE(std::shared_ptr<joynr::Request>)
#endif // REQUEST_H
