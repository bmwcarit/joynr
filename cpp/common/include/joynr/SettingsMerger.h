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
#ifndef QSETTINGSMERGER_H
#define QSETTINGSMERGER_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrCommonExport.h"

#include <QString>
#include <QSettings>

namespace joynr
{

class JOYNRCOMMON_EXPORT SettingsMerger
{

public:
    static QSettings* mergeSettings(QString fileName, QSettings* currentSettings = NULL);
    static void mergeSettings(const QSettings& from, QSettings& into, bool override);

private:
    DISALLOW_COPY_AND_ASSIGN(SettingsMerger);
};

} // namespace joynr
#endif // QSETTINGSMERGER_H
