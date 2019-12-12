/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "HttpResult.h"

#include <boost/format.hpp>
#include <curl/curl.h>

namespace joynr
{

HttpResult::HttpResult(std::int32_t curlError,
                       std::int64_t statusCode,
                       std::string* body,
                       std::unordered_multimap<std::string, std::string>* headers)
        : _curlError(curlError), _statusCode(statusCode), _body(body), _headers(headers)
{
}

bool HttpResult::isCurlError() const
{
    return (_curlError != 0);
}

std::int32_t HttpResult::getCurlError() const
{
    return _curlError;
}

std::int64_t HttpResult::getStatusCode() const
{
    return _statusCode;
}

std::string HttpResult::getErrorMessage() const
{
    if (isCurlError()) {
        switch (_curlError) {
        case CURLE_COULDNT_RESOLVE_PROXY:
            return std::string("Could not resolve network proxy address");
        case CURLE_COULDNT_RESOLVE_HOST:
            return std::string("Could not resolve host address");
        case CURLE_COULDNT_CONNECT:
            return std::string("Curl reported connection failure");
        case CURLE_OPERATION_TIMEDOUT:
            return std::string("Curl operation timeout");
        case CURLE_SSL_CONNECT_ERROR:
            return std::string("SSL connection error");
        default:
            return (boost::format("Error during HTTP request/response, curl error code: %1%: %2%") %
                    _curlError % curl_easy_strerror(static_cast<CURLcode>(_curlError))).str();
        }
    } else {
        switch (_statusCode) {
        case 407:
            return std::string("407 Proxy authentication required");
        case 500:
            return std::string("500 Internal server error");
        case 502:
            return std::string("502 Bad gateway");
        case 503:
            return std::string("503 Service unavailable");
        default:
            return (boost::format("HTTP error, status code : %1%") % _statusCode).str();
        }
    }
}

const std::string& HttpResult::getBody() const
{
    return *_body;
}

const std::unordered_multimap<std::string, std::string>& HttpResult::getHeaders() const
{
    return *_headers;
}

} // namespace joynr
