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
#include <qglobal.h>

namespace joynr
{

/**
  * \class TypeUtil
  * \brief Container class for helper methods related with the used datatypes
  */
class JOYNRCOMMON_EXPORT TypeUtil
{
public:
    /**
      * Converts a QString object into a std::string object
      */
    static std::string toStd(const QString& qtString)
    {
        return qtString.toStdString();
    }

    /**
      * Converts a list of QString objects into a list of std::string objects
      */
    static QList<std::string> toStd(const QList<QString>& qtStrings)
    {
        QList<std::string> stdStrings;

        for (QString qtString : qtStrings) {
            stdStrings.append(toStd(qtString));
        }

        return stdStrings;
    }

    /**
      * Converts a list of std::string objects into a list of QString objects
      */
    static QList<QString> toQt(const QList<std::string>& stdStrings)
    {
        QList<QString> qtStrings;

        for (std::string stdString : stdStrings) {
            qtStrings.append(toQt(stdString));
        }

        return qtStrings;
    }

    /**
      * Converts a std::string object into a QString object
      */
    static QString toQt(const std::string& stdString)
    {
        return QString::fromStdString(stdString);
    }
    /**
      * Converts a qint8 into a uint8_t
      */
    static uint8_t toStdUInt8(const qint8& qtValue)
    {
        return static_cast<uint8_t>(qtValue);
    }

    /**
      * Converts a list of qint8 values into a list of uint8_t objects
      */
    static QList<uint8_t> toStdUInt8(const QList<qint8>& qtValues)
    {
        QList<uint8_t> stdValues;

        for (qint8 qtValue : qtValues) {
            stdValues.append(toStdUInt8(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of uint8_t objects into a list of qint8 objects
      */
    static QList<qint8> toQt(const QList<uint8_t>& stdValues)
    {
        QList<qint8> qtValues;

        for (uint8_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a uint8_t object into a qint8 object
      */
    static qint8 toQt(const uint8_t& stdValue)
    {
        return static_cast<qint8>(stdValue);
    }
};
} // namespace joynr
#endif /* TYPE_UTIL_H_ */
