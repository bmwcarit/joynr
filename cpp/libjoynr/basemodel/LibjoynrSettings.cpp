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

LibjoynrSettings::LibjoynrSettings(Settings& settings) : _settings(settings)
{
    checkSettings();
}

void LibjoynrSettings::checkSettings()
{
    // set default values
    if (!_settings.contains(SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME())) {
        setBroadcastSubscriptionRequestPersistenceFilename(
                DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME())) {
        setMessageRouterPersistenceFilename(DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME())) {
        setParticipantIdsPersistenceFilename(DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME())) {
        setSubscriptionRequestPersistenceFilename(
                DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
    }

    if (!_settings.contains(SETTING_MESSAGE_ROUTER_PERSISTENCY_ENABLED())) {
        setMessageRouterPersistencyEnabled(DEFAULT_MESSAGE_ROUTER_PERSISTENCY_ENABLED());
    }

    if (!_settings.contains(SETTING_SUBSCRIPTION_PERSISTENCY_ENABLED())) {
        setSubscriptionPersistencyEnabled(DEFAULT_SUBSCRIPTION_PERSISTENCY_ENABLED());
    }

    if (!_settings.contains(SETTING_CLEAR_SUBSCRIPTION_ENABLED())) {
        setClearSubscriptionEnabled(DEFAULT_CLEAR_SUBSCRIPTION_ENABLED());
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

const std::string& LibjoynrSettings::SETTING_MESSAGE_ROUTER_PERSISTENCY_ENABLED()
{
    static const std::string value("lib-joynr/message-router-persistency");
    return value;
}

bool LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCY_ENABLED()
{
    return false;
}

const std::string& LibjoynrSettings::SETTING_SUBSCRIPTION_PERSISTENCY_ENABLED()
{
    static const std::string value("lib-joynr/subscription-persistency");
    return value;
}

bool LibjoynrSettings::DEFAULT_SUBSCRIPTION_PERSISTENCY_ENABLED()
{
    return true;
}

const std::string& LibjoynrSettings::SETTING_CLEAR_SUBSCRIPTION_ENABLED()
{
    static const std::string value("lib-joynr/clear-subscription-enabled");
    return value;
}

bool LibjoynrSettings::DEFAULT_CLEAR_SUBSCRIPTION_ENABLED()
{
    return false;
}

std::string LibjoynrSettings::getBroadcastSubscriptionRequestPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setBroadcastSubscriptionRequestPersistenceFilename(
        const std::string& filename)
{
    _settings.set(SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME(), filename);
}

std::string LibjoynrSettings::getMessageRouterPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setMessageRouterPersistenceFilename(const std::string& filename)
{
    _settings.set(SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME(), filename);
}

std::string LibjoynrSettings::getParticipantIdsPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setParticipantIdsPersistenceFilename(const std::string& filename)
{
    _settings.set(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(), filename);
}

std::string LibjoynrSettings::getSubscriptionRequestPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setSubscriptionRequestPersistenceFilename(const std::string& filename)
{
    _settings.set(SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME(), filename);
}

bool LibjoynrSettings::isMessageRouterPersistencyEnabled() const
{
    return _settings.get<bool>(SETTING_MESSAGE_ROUTER_PERSISTENCY_ENABLED());
}

void LibjoynrSettings::setMessageRouterPersistencyEnabled(bool enable)
{
    _settings.set(SETTING_MESSAGE_ROUTER_PERSISTENCY_ENABLED(), enable);
}

bool LibjoynrSettings::isSubscriptionPersistencyEnabled() const
{
    return _settings.get<bool>(SETTING_SUBSCRIPTION_PERSISTENCY_ENABLED());
}

void LibjoynrSettings::setSubscriptionPersistencyEnabled(bool enable)
{
    _settings.set(SETTING_SUBSCRIPTION_PERSISTENCY_ENABLED(), enable);
}

void LibjoynrSettings::setClearSubscriptionEnabled(bool enable)
{
    _settings.set(SETTING_CLEAR_SUBSCRIPTION_ENABLED(), enable);
}

bool LibjoynrSettings::isClearSubscriptionEnabled() const
{
    return _settings.get<bool>(SETTING_CLEAR_SUBSCRIPTION_ENABLED());
}

void LibjoynrSettings::printSettings() const
{
    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_MESSAGE_ROUTER_PERSISTENCY_ENABLED(),
                   isMessageRouterPersistencyEnabled());

    if (isMessageRouterPersistencyEnabled()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME(),
                       getMessageRouterPersistenceFilename());
    }

    if (isClearSubscriptionEnabled()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_CLEAR_SUBSCRIPTION_ENABLED(),
                       isClearSubscriptionEnabled());
    }

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_SUBSCRIPTION_PERSISTENCY_ENABLED(),
                   isSubscriptionPersistencyEnabled());

    if (isSubscriptionPersistencyEnabled()) {
        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME(),
                       getSubscriptionRequestPersistenceFilename());

        JOYNR_LOG_INFO(logger(),
                       "SETTING: {} = {}",
                       SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME(),
                       getBroadcastSubscriptionRequestPersistenceFilename());
    }

    JOYNR_LOG_INFO(logger(),
                   "SETTING: {} = {}",
                   SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(),
                   _settings.get<std::string>(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME()));
}

} // namespace joynr
