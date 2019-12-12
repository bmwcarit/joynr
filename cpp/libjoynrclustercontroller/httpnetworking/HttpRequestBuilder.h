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
#ifndef HTTPREQUESTBUILDER_H
#define HTTPREQUESTBUILDER_H

#include <chrono>
#include <string>

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

#include "HttpNetworking.h"

struct curl_slist;

namespace joynr
{

/**
  * Implementation for all builder interfaces.
  *
  * ATTENTION: Changes to this class might have a side effect on PooledCurlHandle::clearHandle.
  * If some curl property is set inside this builder it must be reset in
  *PooledCurlHandle::clearHandle.
  */
class HttpRequestBuilder : public IHttpGetBuilder,
                           public IHttpPostBuilder,
                           public IHttpDeleteBuilder
{
public:
    explicit HttpRequestBuilder(const std::string& url);
    ~HttpRequestBuilder() override;

    HttpRequest* build() override;
    HttpRequestBuilder* withProxy(const std::string& proxy) override;
    HttpRequestBuilder* withDebug() override;
    HttpRequestBuilder* withCertificateAuthority(const std::string& caFile) override;
    HttpRequestBuilder* withClientCertificate(const std::string& certificateFile) override;
    HttpRequestBuilder* withClientCertificatePassword(const std::string& password) override;
    HttpRequestBuilder* acceptGzip() override;
    HttpRequestBuilder* withConnectTimeout(std::chrono::milliseconds timeout) override;
    HttpRequestBuilder* withTimeout(std::chrono::milliseconds timeout) override;
    HttpRequestBuilder* withContentType(const std::string& contentType) override;
    HttpRequestBuilder* addHeader(const std::string& name, const std::string& value) override;
    HttpRequestBuilder* asPost();
    HttpRequestBuilder* asDelete();
    HttpRequestBuilder* postContent(const std::string& data) override;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpRequestBuilder);
    void* handle;
    curl_slist* headers;

    std::string content;
    bool built;

    ADD_LOGGER(HttpRequestBuilder)
};

} // namespace joynr
#endif // HTTPREQUESTBUILDER_H
