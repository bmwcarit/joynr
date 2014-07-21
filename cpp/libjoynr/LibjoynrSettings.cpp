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
#include "joynr/LibjoynrSettings.h"
#include <QtCore>

namespace joynr {

using namespace joynr_logging;

Logger* LibjoynrSettings::logger = Logging::getInstance()->getLogger("MSG", "LibjoynrSettings");

LibjoynrSettings::LibjoynrSettings(QSettings& settings, QObject *parent) :
    QObject(parent),
    settings(settings)
{
    checkSettings();
}

LibjoynrSettings::LibjoynrSettings(const LibjoynrSettings& other) :
    QObject(other.parent()),
    settings(other.settings)
{
}

LibjoynrSettings::~LibjoynrSettings () {
}

void LibjoynrSettings::checkSettings() const {
    // set default values
    if(!settings.contains(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME())) {
        settings.setValue(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(), DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }
}

const QString& LibjoynrSettings::SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME() {
    static const QString value("lib-joynr/participantids-persistence-file");
    return value;
}

const QString& LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME() {
    static const QString value("joynr_participantIds.settings");
    return value;
}

const QString& LibjoynrSettings::DEFAULT_SUBSCIPTIONREQUEST_STORAGE_FILENAME() {
    static const QString value("SubscriptionRequests.persist");
    return value;
}

QString LibjoynrSettings::getParticipantIdsPersistenceFilename() const {
    return settings.value(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME()).toString();
}

void LibjoynrSettings::setParticipantIdsPersistenceFilename(const QString& filename) {
    settings.setValue(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(), filename);
}

void LibjoynrSettings::printSettings() const {
    LOG_DEBUG(logger, "SETTING: " + SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME() + " = " + settings.value(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME()).toString());
}

} // namespace joynr
