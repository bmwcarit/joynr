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
#ifndef HTTPRESULT_H_
#define HTTPRESULT_H_

#include "joynr/JoynrClusterControllerExport.h"
#include <QMultiMap>
#include <string>
#include <QByteArray>
#include <cstdint>
#include <memory>

namespace joynr
{

/**
  * Encapsulates the result of an http request. Stores the body, the returned headers and the status
 * code.
  * In case of a network error the status code contains the CURL error code instead.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpResult
{
public:
    HttpResult(std::int32_t curlError,
               std::int32_t statusCode,
               QByteArray* body,
               QMultiMap<std::string, std::string>* headers);
    ~HttpResult() = default;

    bool isCurlError() const;
    std::int32_t getCurlError() const;
    std::int32_t getStatusCode() const;
    std::string getErrorMessage() const;
    const QByteArray& getBody() const;
    const QMultiMap<std::string, std::string>& getHeaders() const;

private:
    std::int32_t curlError;
    std::int32_t statusCode;
    std::shared_ptr<QByteArray> body;
    std::shared_ptr<QMultiMap<std::string, std::string>> headers;
};

} // namespace joynr
#endif // HTTPRESULT_H_
