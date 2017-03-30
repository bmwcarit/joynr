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
#include "libjoynrclustercontroller/httpnetworking/HttpNetworking.h"
#include "libjoynrclustercontroller/httpnetworking/HttpRequestBuilder.h"
#include "libjoynrclustercontroller/httpnetworking/CurlHandlePool.h"

#include <curl/curl.h>

namespace joynr
{

HttpNetworking* HttpNetworking::httpNetworking = new HttpNetworking();

HttpNetworking::HttpNetworking()
        : curlHandlePool(std::make_shared<PerThreadCurlHandlePool>()),
          proxy(),
          connectTimeout(std::chrono::milliseconds::zero()),
          certificateAuthority(),
          clientCertificate(),
          clientCertificatePassword(),
          httpDebug(false)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

HttpNetworking* HttpNetworking::getInstance()
{
    return httpNetworking;
}

HttpRequestBuilder* HttpNetworking::createRequestBuilder(const std::string& url)
{
    HttpRequestBuilder* requestBuilder = new HttpRequestBuilder(url);
    if (!proxy.empty()) {
        requestBuilder->withProxy(proxy);
    }
    if (httpDebug) {
        requestBuilder->withDebug();
    }
    // Set the connect timeout
    requestBuilder->withConnectTimeout(connectTimeout);

    // Check for HTTPS options
    if (!certificateAuthority.empty()) {
        requestBuilder->withCertificateAuthority(certificateAuthority);
    }

    if (!clientCertificate.empty()) {
        requestBuilder->withClientCertificate(clientCertificate);
    }

    if (!clientCertificatePassword.empty()) {
        requestBuilder->withClientCertificatePassword(clientCertificatePassword);
    }

    return requestBuilder;
}

IHttpGetBuilder* HttpNetworking::createHttpGetBuilder(const std::string& url)
{
    return createRequestBuilder(url);
}

IHttpDeleteBuilder* HttpNetworking::createHttpDeleteBuilder(const std::string& url)
{
    return createRequestBuilder(url)->asDelete();
}

IHttpPostBuilder* HttpNetworking::createHttpPostBuilder(const std::string& url)
{
    return createRequestBuilder(url)->asPost();
}

void HttpNetworking::setGlobalProxy(const std::string& proxy)
{
    this->proxy = proxy;
}

void HttpNetworking::setHTTPDebugOn()
{
    this->httpDebug = true;
}

void HttpNetworking::setConnectTimeout(std::chrono::milliseconds connectTimeout)
{
    this->connectTimeout = connectTimeout;
}

void HttpNetworking::setCertificateAuthority(const std::string& certificateAuthority)
{
    this->certificateAuthority = certificateAuthority;
}

void HttpNetworking::setClientCertificate(const std::string& clientCertificate)
{
    this->clientCertificate = clientCertificate;
}

void HttpNetworking::setClientCertificatePassword(const std::string& clientCertificatePassword)
{
    this->clientCertificatePassword = clientCertificatePassword;
}

std::shared_ptr<ICurlHandlePool> HttpNetworking::getCurlHandlePool() const
{
    return curlHandlePool;
}

} // namespace joynr
