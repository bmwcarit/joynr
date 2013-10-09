/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef HTTPRESULT_H_
#define HTTPRESULT_H_

#include "joynr/JoynrClusterControllerExport.h"
#include <QMultiMap>
#include <QSharedPointer>
#include <QString>

namespace joynr {

/**
  * Encapsulates the result of an http request. Stores the body, the returned headers and the status code.
  * In case of a network error the status code contains the CURL error code instead.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpResult {
public:
    HttpResult(long curlError, int statusCode, QByteArray* body, QMultiMap<QString, QString>* headers);
    ~HttpResult();

    bool isCurlError() const;
    int getCurlError() const;
    int getStatusCode() const;
    QString getErrorMessage() const;
    const QByteArray& getBody() const;
    const QMultiMap<QString, QString>& getHeaders() const;

private:
    int curlError;
    int statusCode;
    QSharedPointer<QByteArray> body;
    QSharedPointer<QMultiMap<QString, QString> > headers;
};


} // namespace joynr
#endif //HTTPRESULT_H_
