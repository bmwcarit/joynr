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
      * Converts a qint8 into a int8_t
      */
    static int8_t toStdInt8(const qint8& qtValue)
    {
        return static_cast<int8_t>(qtValue);
    }

    /**
      * Converts a list of qint8 values into a list of int8_t objects
      */
    static QList<int8_t> toStdInt8(const QList<qint8>& qtValues)
    {
        QList<int8_t> stdValues;

        for (qint8 qtValue : qtValues) {
            stdValues.append(toStdInt8(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of int8_t objects into a list of qint8 objects
      */
    static QList<qint8> toQt(const QList<int8_t>& stdValues)
    {
        QList<qint8> qtValues;

        for (int8_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a int8_t object into a qint8 object
      */
    static qint8 toQt(const int8_t& stdValue)
    {
        return static_cast<qint8>(stdValue);
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

    /**
      * Converts a int into a int16_t
      */
    static int16_t toStdInt16(const int& qtValue)
    {
        return static_cast<int16_t>(qtValue);
    }

    /**
      * Converts a list of int values into a list of int16_t objects
      */
    static QList<int16_t> toStdInt16(const QList<int>& qtValues)
    {
        QList<int16_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.append(toStdInt16(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of int16_t objects into a list of int objects
      */
    static QList<int> toQt(const QList<int16_t>& stdValues)
    {
        QList<int> qtValues;

        for (int16_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a int16_t object into a int object
      */
    static int toQt(const int16_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a int into a uint16_t
      */
    static uint16_t toStdUInt16(const int& qtValue)
    {
        return static_cast<uint16_t>(qtValue);
    }

    /**
      * Converts a list of int values into a list of uint16_t objects
      */
    static QList<uint16_t> toStdUInt16(const QList<int>& qtValues)
    {
        QList<uint16_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.append(toStdUInt16(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of uint16_t objects into a list of int objects
      */
    static QList<int> toQt(const QList<uint16_t>& stdValues)
    {
        QList<int> qtValues;

        for (uint16_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a uint16_t object into a int object
      */
    static int toQt(const uint16_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a int into a int32_t
      */
    static int32_t toStdInt32(const int& qtValue)
    {
        return static_cast<int32_t>(qtValue);
    }

    /**
      * Converts a list of int values into a list of int32_t objects
      */
    static QList<int32_t> toStdInt32(const QList<int>& qtValues)
    {
        QList<int32_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.append(toStdInt32(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of int32_t objects into a list of int objects
      */
    static QList<int> toQt(const QList<int32_t>& stdValues)
    {
        QList<int> qtValues;

        for (int32_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a int32_t object into a int object
      */
    static int toQt(const int32_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a int into a uint32_t
      */
    static uint32_t toStdUInt32(const int& qtValue)
    {
        return static_cast<uint32_t>(qtValue);
    }

    /**
      * Converts a list of int values into a list of uint32_t objects
      */
    static QList<uint32_t> toStdUInt32(const QList<int>& qtValues)
    {
        QList<uint32_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.append(toStdUInt32(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of uint32_t objects into a list of int objects
      */
    static QList<int> toQt(const QList<uint32_t>& stdValues)
    {
        QList<int> qtValues;

        for (uint32_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a uint32_t object into a int object
      */
    static int toQt(const uint32_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }
};
} // namespace joynr
#endif /* TYPE_UTIL_H_ */
