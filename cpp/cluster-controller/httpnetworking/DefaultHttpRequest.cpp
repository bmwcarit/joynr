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
#include "cluster-controller/httpnetworking/DefaultHttpRequest.h"
#include "cluster-controller/httpnetworking/HttpResult.h"
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>

namespace joynr
{

INIT_LOGGER(DefaultHttpRequest);

size_t DefaultHttpRequest::writeToQByteArray(void* buffer, size_t size, size_t nmemb, void* userp)
{
    QByteArray* data = reinterpret_cast<QByteArray*>(userp);
    size_t numBytes = size * nmemb;
    if (size > 0) {
        data->append(reinterpret_cast<char*>(buffer), numBytes);
    }
    return numBytes;
}

size_t DefaultHttpRequest::writeToQMultiMap(void* buffer, size_t size, size_t nmemb, void* userp)
{
    QMultiMap<std::string, std::string>* headers =
            reinterpret_cast<QMultiMap<std::string, std::string>*>(userp);
    size_t numBytes = size * nmemb;
    std::string header = std::string(reinterpret_cast<char*>(buffer), numBytes);
    std::string::size_type separatorPosition = header.find(":", 0);
    std::string headerName = header.substr(0, separatorPosition);
    std::string headerValue = header.substr(separatorPosition + 1);
    using boost::algorithm::trim;
    trim(headerName);
    trim(headerValue);
    headers->insert(headerName, headerValue);
    return numBytes;
}

DefaultHttpRequest::DefaultHttpRequest(void* handle, const QByteArray& content, curl_slist* headers)
        : handle(handle), headers(headers), content(content)
{
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToQByteArray);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeToQMultiMap);

    if (headers != nullptr) {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }

    if (!this->content.isEmpty()) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, this->content.data());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, this->content.size());
    }
}

DefaultHttpRequest::~DefaultHttpRequest()
{
    if (headers != nullptr) {
        curl_slist_free_all(headers);
    }
    if (handle != nullptr) {
        HttpNetworking::getInstance()->getCurlHandlePool()->returnHandle(handle);
    }
}

HttpResult DefaultHttpRequest::execute()
{
    QByteArray* body = new QByteArray;
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, body);

    QMultiMap<std::string, std::string>* headers = new QMultiMap<std::string, std::string>;
    curl_easy_setopt(handle, CURLOPT_WRITEHEADER, headers);

    CURLcode curlError;
    curlError = curl_easy_perform(handle);

    std::int32_t statusCode;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &statusCode);

    // Check for internal curl errors
    if (curlError == CURLE_FAILED_INIT) {
        // Delete the handle that we were using
        HttpNetworking::getInstance()->getCurlHandlePool()->deleteHandle(handle);
        handle = nullptr;
    }

    return HttpResult(curlError, statusCode, body, headers);
}

} // namespace joynr
