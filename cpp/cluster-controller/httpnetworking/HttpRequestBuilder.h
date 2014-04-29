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
#ifndef HTTPREQUESTBUILDER_H_
#define HTTPREQUESTBUILDER_H_
#include "joynr/PrivateCopyAssign.h"

#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "joynr/joynrlogging.h"

#include <QByteArray>

struct curl_slist;

namespace joynr {

/**
  * Implementation for all builder interfaces.
  *
  * ATTENTION: Changes to this class might have a side effect on PooledCurlHandle::clearHandle.
  * If some curl property is set inside this builder it must be reset in PooledCurlHandle::clearHandle.
  */
class HttpRequestBuilder : public IHttpGetBuilder, public IHttpPostBuilder, public IHttpDeleteBuilder {
public:
    HttpRequestBuilder(const QString& url);
    ~HttpRequestBuilder();

    HttpRequest* build();
    HttpRequestBuilder* withProxy(const QString& proxy);
    HttpRequestBuilder* withDebug();
    HttpRequestBuilder* acceptGzip();
    HttpRequestBuilder* withConnectTimeout_ms(long timeout_ms);
    HttpRequestBuilder* withTimeout_ms(long timeout_ms);
    HttpRequestBuilder* withContentType(const QString& contentType);
    HttpRequestBuilder* addHeader(const QString& name, const QString& value);
    HttpRequestBuilder* asPost();
    HttpRequestBuilder* asDelete();
    HttpRequestBuilder* postContent(const QByteArray& data);

private:
    DISALLOW_COPY_AND_ASSIGN(HttpRequestBuilder);
    void* handle;
    curl_slist* headers;

    QByteArray content;
    bool built;

    static joynr_logging::Logger* logger;
};



} // namespace joynr
#endif //HTTPREQUESTBUILDER_H_
