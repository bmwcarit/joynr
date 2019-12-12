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

#include "DefaultHttpRequest.h"

#include <cstdint>
#include <unordered_map>

#include <boost/algorithm/string.hpp>

#include <curl/curl.h>

#include "HttpResult.h"

namespace joynr
{

size_t DefaultHttpRequest::writeToString(void* buffer, size_t size, size_t nmemb, void* userp)
{
    std::string* data = reinterpret_cast<std::string*>(userp);
    size_t numBytes = size * nmemb;
    if (size > 0) {
        data->append(reinterpret_cast<char*>(buffer), numBytes);
    }
    return numBytes;
}

size_t DefaultHttpRequest::writeToMultiMap(void* buffer, size_t size, size_t nmemb, void* userp)
{
    std::unordered_multimap<std::string, std::string>* headers =
            reinterpret_cast<std::unordered_multimap<std::string, std::string>*>(userp);
    size_t numBytes = size * nmemb;
    std::string header = std::string(reinterpret_cast<char*>(buffer), numBytes);
    std::string::size_type separatorPosition = header.find(":", 0);
    std::string headerName = header.substr(0, separatorPosition);
    std::string headerValue = header.substr(separatorPosition + 1);
    using boost::algorithm::trim;
    trim(headerName);
    trim(headerValue);
    headers->insert({std::move(headerName), std::move(headerValue)});
    return numBytes;
}

DefaultHttpRequest::DefaultHttpRequest(void* handle,
                                       const std::string& content,
                                       curl_slist* headers)
        : _handle(handle), _headers(headers), _content(content)
{
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeToMultiMap);

    if (headers != nullptr) {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }

    if (!this->_content.empty()) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, this->_content.data());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, this->_content.size());
    }
}

DefaultHttpRequest::~DefaultHttpRequest()
{
    if (_headers != nullptr) {
        curl_slist_free_all(_headers);
    }
    if (_handle != nullptr) {
        HttpNetworking::getInstance()->getCurlHandlePool()->returnHandle(_handle);
    }
}

HttpResult DefaultHttpRequest::execute()
{
    std::string* body = new std::string;
    curl_easy_setopt(_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(_handle, CURLOPT_WRITEDATA, body);

    auto headers = new std::unordered_multimap<std::string, std::string>();
    curl_easy_setopt(_handle, CURLOPT_WRITEHEADER, headers);

    CURLcode curlError;
    curlError = curl_easy_perform(_handle);

    std::int64_t statusCode = 0;
    curl_easy_getinfo(_handle, CURLINFO_RESPONSE_CODE, &statusCode);

    // Check for internal curl errors
    if (curlError == CURLE_FAILED_INIT) {
        // Delete the handle that we were using
        HttpNetworking::getInstance()->getCurlHandlePool()->deleteHandle(_handle);
        _handle = nullptr;
    }

    return HttpResult(curlError, statusCode, body, headers);
}

void DefaultHttpRequest::interrupt()
{
    if (_handle != nullptr) {
        curl_easy_setopt(_handle, CURLOPT_TIMEOUT_MS, 1);
    }
}

} // namespace joynr
