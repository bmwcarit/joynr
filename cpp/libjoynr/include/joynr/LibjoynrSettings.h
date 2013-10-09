/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef LIBJOYNRSETTINGS_H
#define LIBJOYNRSETTINGS_H

#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"

#include <QObject>
#include <QSettings>


namespace joynr {

class JOYNR_EXPORT LibjoynrSettings : public QObject {
    Q_OBJECT

public:
    static const QString& SETTING_PARTICIPANT_IDS_PERSISTENCE_FILENAME();
    static const QString& DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME();

    explicit LibjoynrSettings(QSettings& settings, QObject* parent = 0);
    LibjoynrSettings(const LibjoynrSettings& other);

    ~LibjoynrSettings();

    QString getParticipantIdsPersistenceFilename() const;
    void setParticipantIdsPersistenceFilename(const QString& persistenceFilename);

    void printSettings() const;

private:
    void operator =(const LibjoynrSettings &other);

    QSettings& settings;
    static joynr_logging::Logger* logger;
    void checkSettings() const;
};


} // namespace joynr
#endif // LIBJOYNRSETTINGS_H
