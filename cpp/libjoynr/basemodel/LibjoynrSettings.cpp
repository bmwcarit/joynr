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
    if (!_settings.contains(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME())) {
        setParticipantIdsPersistenceFilename(DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }
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

std::string LibjoynrSettings::getParticipantIdsPersistenceFilename() const
{
    return _settings.get<std::string>(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
}

void LibjoynrSettings::setParticipantIdsPersistenceFilename(const std::string& filename)
{
    _settings.set(SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME(), filename);
}

void LibjoynrSettings::printSettings() const
{
}

} // namespace joynr
