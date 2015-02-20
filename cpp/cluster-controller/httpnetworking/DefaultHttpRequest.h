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
#ifndef DEFAULTHTTPREQUEST_H_
#define DEFAULTHTTPREQUEST_H_
#include "joynr/PrivateCopyAssign.h"

#include "cluster-controller/httpnetworking/HttpNetworking.h"
#include "joynr/joynrlogging.h"
#include <QByteArray>

struct curl_slist;

namespace joynr
{

/**
  * This class encapsulates the execution of an http request.
  */
class DefaultHttpRequest : public HttpRequest
{
public:
    DefaultHttpRequest(void* handle, const QByteArray& content, curl_slist* headers);
    HttpResult execute();
    virtual ~DefaultHttpRequest();

private:
    DISALLOW_COPY_AND_ASSIGN(DefaultHttpRequest);
    static size_t writeToQByteArray(void* buffer, size_t size, size_t nmemb, void* userp);
    static size_t writeToQMultiMap(void* buffer, size_t size, size_t nmemb, void* userp);

    void* handle;
    curl_slist* headers;

    QByteArray content;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // DEFAULTHTTPREQUEST_H_
