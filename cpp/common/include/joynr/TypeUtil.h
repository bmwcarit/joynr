/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef TYPE_UTIL_H_
#define TYPE_UTIL_H_

#include "joynr/JoynrCommonExport.h"

#include <QString>
#include <string>
#include <QList>

namespace joynr
{

/**
  * \class TypeUtil
  * \brief Container class for helper methods related with the used datatyps
  */
class JOYNRCOMMON_EXPORT TypeUtil
{
public:
    /**
      * Converts a QString object into a std::string object
      */
    static std::string convertQStringtoStdString(const QString& qtString)
    {
        return qtString.toStdString();
    }

    /**
      * Converts a list of QString objects into a list of std::string objects
      */
    static QList<std::string> convertQStringstoStdStrings(const QList<QString>& qtStrings)
    {
        QList<std::string> stdStrings;

        for (QString qtString : qtStrings) {
            stdStrings.append(convertQStringtoStdString(qtString));
        }

        return stdStrings;
    }

    /**
      * Converts a list of std::string objects into a list of QString objects
      */
    static QList<QString> convertStdStringstoQStrings(const QList<std::string>& stdStrings)
    {
        QList<QString> qtStrings;

        for (std::string stdString : stdStrings) {
            qtStrings.append(QString::fromStdString(stdString));
        }

        return qtStrings;
    }

    /**
      * Converts a std::string object into a QString object
      */
    static QString convertStdStringtoQString(const std::string& stdString)
    {
        return QString::fromStdString(stdString);
    }
};
} // namespace joynr
#endif /* TYPE_UTIL_H_ */
