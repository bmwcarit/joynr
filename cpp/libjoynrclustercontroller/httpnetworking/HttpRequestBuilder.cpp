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
#include "HttpRequestBuilder.h"

#include <curl/curl.h>

#include "joynr/exceptions/JoynrException.h"

#include "DefaultHttpRequest.h"

namespace joynr
{

HttpRequestBuilder::HttpRequestBuilder(const std::string& url)
        : handle(nullptr), headers(nullptr), content(), built(false)
{
    handle = HttpNetworking::getInstance()->getCurlHandlePool()->getHandle(url);
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
}

HttpRequestBuilder::~HttpRequestBuilder()
{
    if (!built) {
        if (headers != nullptr) {
            curl_slist_free_all(headers);
        }
        HttpNetworking::getInstance()->getCurlHandlePool()->returnHandle(handle);
    }
}

HttpRequestBuilder* HttpRequestBuilder::withProxy(const std::string& proxy)
{
    curl_easy_setopt(handle, CURLOPT_PROXY, proxy.c_str());
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withCertificateAuthority(const std::string& caFile)
{
    curl_easy_setopt(handle, CURLOPT_CAINFO, caFile.c_str());
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withClientCertificate(const std::string& certificateFile)
{
    curl_easy_setopt(handle, CURLOPT_SSLCERT, certificateFile.c_str());
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withClientCertificatePassword(const std::string& password)
{
    curl_easy_setopt(handle, CURLOPT_KEYPASSWD, password.c_str());
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withDebug()
{
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::postContent(const std::string& data)
{
    content = data;
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::asPost()
{
    curl_easy_setopt(handle, CURLOPT_POST, 1);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, 0);

    // Workaround for server behaviour described here:
    //  http://curl.haxx.se/mail/lib-2011-12/0348.html
    addHeader("Expect", "");
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::asDelete()
{
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withTimeout(std::chrono::milliseconds timeout)
{
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, timeout.count());
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withConnectTimeout(std::chrono::milliseconds timeout)
{
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, timeout.count());
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::acceptGzip()
{
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "gzip");
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::withContentType(const std::string& contentType)
{
    addHeader("Content-Type", contentType);
    return this;
}

HttpRequestBuilder* HttpRequestBuilder::addHeader(const std::string& name, const std::string& value)
{
    std::string header(name + ": " + value);
    headers = curl_slist_append(headers, header.c_str());
    return this;
}

HttpRequest* HttpRequestBuilder::build()
{
    if (built) {
        JOYNR_LOG_WARN(logger(),
                       "The method build of HttpBuilder may be called only once on a specific "
                       "instance. Throwing an Exception from worker thread.");
        throw exceptions::JoynrRuntimeException(
                "The method build of HttpBuilder may be called only once on a specific instance");
    }
    built = true;
    return new DefaultHttpRequest(handle, content, headers);
}

} // namespace joynr
