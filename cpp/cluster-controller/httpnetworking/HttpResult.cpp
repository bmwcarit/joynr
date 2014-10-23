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
#include "cluster-controller/httpnetworking/HttpResult.h"
#include <curl/curl.h>

namespace joynr
{

HttpResult::HttpResult(long curlError,
                       int statusCode,
                       QByteArray* body,
                       QMultiMap<QString, QString>* headers)
        : curlError(static_cast<int>(curlError)),
          statusCode(statusCode),
          body(body),
          headers(headers)
{
}

HttpResult::~HttpResult()
{
}

bool HttpResult::isCurlError() const
{
    return (curlError != 0);
}

int HttpResult::getCurlError() const
{
    return curlError;
}

int HttpResult::getStatusCode() const
{
    return statusCode;
}

QString HttpResult::getErrorMessage() const
{
    if (isCurlError()) {
        switch (curlError) {
        case CURLE_COULDNT_RESOLVE_PROXY:
            return QString("Could not resolve network proxy address");
        case CURLE_COULDNT_RESOLVE_HOST:
            return QString("Could not resolve host address");
        case CURLE_COULDNT_CONNECT:
            return QString("Curl reported connection failure");
        case CURLE_OPERATION_TIMEDOUT:
            return QString("Curl operation timeout");
        case CURLE_SSL_CONNECT_ERROR:
            return QString("SSL connection error");
        default:
            return QString("Error during HTTP request/response, curl error code : %1")
                    .arg(curlError);
        }
    } else {
        switch (statusCode) {
        case 407:
            return QString("407 Proxy authentication required");
        case 500:
            return QString("500 Internal server error");
        case 502:
            return QString("502 Bad gateway");
        case 503:
            return QString("503 Service unavailable");
        default:
            return QString("HTTP error, status code : %1").arg(statusCode);
        }
    }
}

const QByteArray& HttpResult::getBody() const
{
    return *body;
}

const QMultiMap<QString, QString>& HttpResult::getHeaders() const
{
    return *headers;
}

} // namespace joynr
