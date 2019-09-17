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
#ifndef DEFAULTHTTPREQUEST_H
#define DEFAULTHTTPREQUEST_H

#include <string>

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "libjoynrclustercontroller/httpnetworking/HttpNetworking.h"

struct curl_slist;

namespace joynr
{

/**
  * This class encapsulates the execution of an http request.
  */
class DefaultHttpRequest : public HttpRequest
{
public:
    DefaultHttpRequest(void* _handle, const std::string& content, curl_slist* headers);
    HttpResult execute() override;
    void interrupt() override;
    ~DefaultHttpRequest() override;

private:
    DISALLOW_COPY_AND_ASSIGN(DefaultHttpRequest);
    static size_t writeToString(void* buffer, size_t size, size_t nmemb, void* userp);
    static size_t writeToMultiMap(void* buffer, size_t size, size_t nmemb, void* userp);

    void* _handle;
    curl_slist* _headers;

    std::string _content;
    ADD_LOGGER(DefaultHttpRequest)
};

} // namespace joynr
#endif // DEFAULTHTTPREQUEST_H
