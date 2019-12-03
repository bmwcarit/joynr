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
#include "libjoynrclustercontroller/httpnetworking/CurlHandlePool.h"
#include "libjoynrclustercontroller/httpnetworking/HttpRequestBuilder.h"

#include <curl/curl.h>

namespace joynr
{

HttpNetworking::HttpNetworking()
        : _curlHandlePool(std::make_shared<PerThreadCurlHandlePool>()),
          _proxy(),
          _connectTimeout(std::chrono::milliseconds::zero()),
          _certificateAuthority(),
          _clientCertificate(),
          _clientCertificatePassword(),
          _httpDebug(false)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

std::shared_ptr<HttpNetworking> HttpNetworking::getInstance()
{
    static std::shared_ptr<HttpNetworking> instance(new HttpNetworking());
    return instance;
}

HttpRequestBuilder* HttpNetworking::createRequestBuilder(const std::string& url)
{
    HttpRequestBuilder* requestBuilder = new HttpRequestBuilder(url);
    if (!_proxy.empty()) {
        requestBuilder->withProxy(_proxy);
    }
    if (_httpDebug) {
        requestBuilder->withDebug();
    }
    // Set the connect timeout
    requestBuilder->withConnectTimeout(_connectTimeout);

    // Check for HTTPS options
    if (!_certificateAuthority.empty()) {
        requestBuilder->withCertificateAuthority(_certificateAuthority);
    }

    if (!_clientCertificate.empty()) {
        requestBuilder->withClientCertificate(_clientCertificate);
    }

    if (!_clientCertificatePassword.empty()) {
        requestBuilder->withClientCertificatePassword(_clientCertificatePassword);
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
    this->_proxy = proxy;
}

void HttpNetworking::setHTTPDebugOn()
{
    this->_httpDebug = true;
}

void HttpNetworking::setConnectTimeout(std::chrono::milliseconds connectTimeout)
{
    this->_connectTimeout = connectTimeout;
}

void HttpNetworking::setCertificateAuthority(const std::string& certificateAuthority)
{
    this->_certificateAuthority = certificateAuthority;
}

void HttpNetworking::setClientCertificate(const std::string& clientCertificate)
{
    this->_clientCertificate = clientCertificate;
}

void HttpNetworking::setClientCertificatePassword(const std::string& clientCertificatePassword)
{
    this->_clientCertificatePassword = clientCertificatePassword;
}

std::shared_ptr<ICurlHandlePool> HttpNetworking::getCurlHandlePool() const
{
    return _curlHandlePool;
}

} // namespace joynr
