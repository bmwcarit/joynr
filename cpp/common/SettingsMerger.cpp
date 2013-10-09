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
#include "SettingsMerger.h"
#include "joynr/Util.h"
#include <QSettings>
#include <QString>
#include <QStringList>

namespace joynr {

QSettings* SettingsMerger::mergeSettings(QString fileName, QSettings* currentSettings) {
    // create new settingsfile
    if(!currentSettings) {
        QString uuid = Util::createUuid();
        currentSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "GENIVI", "Joynr-"+uuid);
    }

    // load new settings file
    QSettings newSettings(fileName, QSettings::IniFormat);

    // iterate over new settings and add if not existing
    QStringList newKeyList = newSettings.allKeys();
    for(int index = 0; index < newKeyList.size(); index++) {
        QString key = newKeyList.at(index);
        // check if key exists
        if(!currentSettings->contains(key)) {
            currentSettings->setValue(key, newSettings.value(key));
        }
    }
    return currentSettings;
}

} // namespace joynr
