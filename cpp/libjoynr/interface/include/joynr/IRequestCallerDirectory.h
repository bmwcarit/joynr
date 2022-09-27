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
#ifndef IREQUESTCALLERDIRECTORY_H
#define IREQUESTCALLERDIRECTORY_H

#include <memory>
#include <string>

namespace joynr
{

class RequestCaller;

class IRequestCallerDirectory
{
public:
    virtual ~IRequestCallerDirectory() = default;
    virtual std::shared_ptr<RequestCaller> lookupRequestCaller(
            const std::string& participantId) = 0;
    virtual bool containsRequestCaller(const std::string& participantId) = 0;
};

} // namespace joynr
#endif // IREQUESTCALLERDIRECTORY_H
