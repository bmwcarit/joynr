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
#include "joynr/LibjoynrSettings.h"

#include "joynr/Logger.h"
#include "joynr/Settings.h"

namespace joynr
{

INIT_LOGGER(LibjoynrSettings);

LibjoynrSettings::LibjoynrSettings(Settings& settings) : settings(settings)
{
    checkSettings();
}

void LibjoynrSettings::checkSettings()
{
    // set default values
    if (!settings.contains(SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME())) {
        setBroadcastSubscriptionRequestPersistenceFilename(
                DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
    }

    if (!settings.contains(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME())) {
        setMessageRouterPersistenceFilename(DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME());
    }

    if (!settings.contains(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME())) {
        setParticipantIdsPersistenceFilename(DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }

    if (!settings.contains(SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME())) {
        setSubscriptionRequestPersistenceFilename(
                DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
    }
}

const std::string& LibjoynrSettings::SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()
{
    static const std::string value("lib-joynr/broadcastsubscriptionrequest-persistence-file");
    return value;
}

const std::string& LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()
{
    static const std::string value("BroadcastSubscriptionRequests.persist");
    return value;
}

const std::string& LibjoynrSettings::SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME()
{
    static const std::string value("lib-joynr/message-router-persistence-file");
    return value;
}

const std::string& LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME()
{
    static const std::string value("MessageRouter.persist");
    return value;
}
const std::string& LibjoynrSettings::SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME()
{
    static const std::string value("lib-joynr/participant-ids-persistence-file");
    return value;
}

const std::string& LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME()
{
    static const std::string value("ParticipantIds.persist");
    return value;
}

const std::string& LibjoynrSettings::SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()
{
    static const std::string value("lib-joynr/subscriptionrequest-persistence-file");
    return value;
}

const std::string& LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()
{
    static const std::string value("SubscriptionRequests.persist");
    return value;
}

std::string LibjoynrSettings::getBroadcastSubscriptionRequestPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setBroadcastSubscriptionRequestPersistenceFilename(
        const std::string& filename)
{
    settings.set(SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME(), filename);
}

std::string LibjoynrSettings::getMessageRouterPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setMessageRouterPersistenceFilename(const std::string& filename)
{
    settings.set(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME(), filename);
}

std::string LibjoynrSettings::getParticipantIdsPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setParticipantIdsPersistenceFilename(const std::string& filename)
{
    settings.set(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(), filename);
}

std::string LibjoynrSettings::getSubscriptionRequestPersistenceFilename() const
{
    return settings.get<std::string>(SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setSubscriptionRequestPersistenceFilename(const std::string& filename)
{
    settings.set(SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME(), filename);
}

void LibjoynrSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME(),
                    settings.get<std::string>(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME()));

    JOYNR_LOG_DEBUG(logger,
                    "SETTING: {}  = {}",
                    SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(),
                    settings.get<std::string>(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME()));
}

} // namespace joynr
