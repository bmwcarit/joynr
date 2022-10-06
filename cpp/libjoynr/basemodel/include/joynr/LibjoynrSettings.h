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
#ifndef LIBJOYNRSETTINGS_H
#define LIBJOYNRSETTINGS_H

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

#include <string>

namespace joynr
{

class Settings;

class JOYNR_EXPORT LibjoynrSettings
{
public:
    static const std::string& SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME();
    static const std::string& DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME();

    explicit LibjoynrSettings(Settings& settings);
    LibjoynrSettings(const LibjoynrSettings&) = default;
    LibjoynrSettings(LibjoynrSettings&&) = default;

    ~LibjoynrSettings() = default;

    std::string getParticipantIdsPersistenceFilename() const;
    void setParticipantIdsPersistenceFilename(const std::string& filename);

    void printSettings() const;

private:
    void operator=(const LibjoynrSettings& other) = delete;

    Settings& _settings;
    ADD_LOGGER(LibjoynrSettings)
    void checkSettings();
};

} // namespace joynr
#endif // LIBJOYNRSETTINGS_H
