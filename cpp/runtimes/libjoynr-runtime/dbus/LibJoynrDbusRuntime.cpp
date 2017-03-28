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
#include "runtimes/libjoynr-runtime/dbus/LibJoynrDbusRuntime.h"

#include "common/dbus/DbusMessagingStubAdapter.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/DBusMessageRouterAdapter.h"
#include "common/dbus/DbusSettings.h"
#include "libjoynr/dbus/DbusMessagingStubFactory.h"
#include "joynr/Util.h"

namespace joynr
{

LibJoynrDbusRuntime::LibJoynrDbusRuntime(std::unique_ptr<Settings> settings)
        : LibJoynrRuntime(std::move(settings)),
          dbusMessageRouterAdapter(nullptr),
          dbusSettings(new DbusSettings(*this->settings)),
          libjoynrMessagingServiceUrl()
{
    dbusSettings->printSettings();

    std::string uuid = Util::createUuid();
    // remove dashes
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
    std::string libjoynrMessagingDomain("local");
    std::string libjoynrMessagingServiceName("io.joynr.libjoynr.Messaging");
    std::string libjoynrMessagingId("libjoynr.messaging.participantid_" + uuid);
    libjoynrMessagingServiceUrl = libjoynrMessagingDomain + ":" + libjoynrMessagingServiceName +
                                  ":" + libjoynrMessagingId;
    auto libjoynrMessagingAddress = std::make_shared<system::RoutingTypes::CommonApiDbusAddress>(
                    libjoynrMessagingDomain, libjoynrMessagingServiceName, libjoynrMessagingId);

    // create connection to parent routing service
    auto ccMessagingAddress = std::make_shared<system::RoutingTypes::CommonApiDbusAddress>(
                    dbusSettings->getClusterControllerMessagingDomain(),
                    dbusSettings->getClusterControllerMessagingServiceName(),
                    dbusSettings->getClusterControllerMessagingParticipantId());

    LibJoynrRuntime::init(
            new DbusMessagingStubFactory(), libjoynrMessagingAddress, ccMessagingAddress);
}

LibJoynrDbusRuntime::~LibJoynrDbusRuntime()
{
    delete dbusMessageRouterAdapter;
    dbusMessageRouterAdapter = nullptr;
    delete dbusSettings;
    dbusSettings = nullptr;
}

void LibJoynrDbusRuntime::startLibJoynrMessagingSkeleton(MessageRouter& messageRouter)
{
    // create messaging skeleton using uuid
    dbusMessageRouterAdapter =
            new DBusMessageRouterAdapter(messageRouter, libjoynrMessagingServiceUrl);
}

} // namespace joynr
