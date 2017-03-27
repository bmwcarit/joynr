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

#ifndef LIBJOYNRDBUSRUNTIME_H
#define LIBJOYNRDBUSRUNTIME_H

#include "joynr/PrivateCopyAssign.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include <string>

namespace joynr
{

class DBusMessageRouterAdapter;
class DbusSettings;
class Settings;

class LibJoynrDbusRuntime : public LibJoynrRuntime
{
public:
    explicit LibJoynrDbusRuntime(std::unique_ptr<Settings> settings);
    ~LibJoynrDbusRuntime() override;

protected:
    DBusMessageRouterAdapter* dbusMessageRouterAdapter;
    DbusSettings* dbusSettings;

    virtual void startLibJoynrMessagingSkeleton(MessageRouter& messageRouter);

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrDbusRuntime);
    std::string libjoynrMessagingServiceUrl;
};

} // namespace joynr
#endif // LIBJOYNRDBUSRUNTIME_H
