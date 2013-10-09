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
#include "cluster-controller/httpnetworking/DefaultHttpRequest.h"
#include "cluster-controller/httpnetworking/HttpResult.h"

#include <curl/curl.h>

namespace joynr {

using namespace joynr_logging;
Logger* DefaultHttpRequest::logger = Logging::getInstance()->getLogger("MSG", "DefaultHttpRequest");

size_t DefaultHttpRequest::writeToQByteArray(void *buffer, size_t size, size_t nmemb, void *userp) {
    QByteArray* data = reinterpret_cast<QByteArray*>(userp);
    size_t numBytes = size * nmemb;
    if(size > 0) {
        data->append(reinterpret_cast<char*>(buffer), numBytes);
    }
    return numBytes;
}

size_t DefaultHttpRequest::writeToQMultiMap(void *buffer, size_t size, size_t nmemb, void *userp) {
    QMultiMap<QString, QString>* headers = reinterpret_cast<QMultiMap<QString, QString>*>(userp);
    size_t numBytes = size * nmemb;
    QString header = QString::fromUtf8(reinterpret_cast<char*>(buffer), numBytes);
    int separatorPosition = header.indexOf(':');
    QString headerName = header.left(separatorPosition).trimmed();
    QString headerValue = header.mid(separatorPosition + 1).trimmed();
    headers->insert(headerName, headerValue);
    return numBytes;
}

DefaultHttpRequest::DefaultHttpRequest(void* handle, const QByteArray& content, curl_slist* headers)
    : handle(handle),
      headers(headers),
      content(content)
{
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToQByteArray);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeToQMultiMap);

    if(headers != 0) {
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }

    if(!this->content.isEmpty()) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, this->content.data());
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, this->content.size());
    }
}

DefaultHttpRequest::~DefaultHttpRequest() {
    if(headers != 0) {
        curl_slist_free_all(headers);
    }
    HttpNetworking::getInstance()->getCurlHandlePool()->returnHandle(handle);
}

HttpResult DefaultHttpRequest::execute() {
    QByteArray* body = new QByteArray;
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, body);

    QMultiMap<QString, QString>* headers = new QMultiMap<QString, QString>;
    curl_easy_setopt(handle, CURLOPT_WRITEHEADER, headers);

    CURLcode curlError;
    curlError = curl_easy_perform(handle);

    long statusCode;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &statusCode);

    return HttpResult(curlError, statusCode, body, headers);
}

} // namespace joynr
