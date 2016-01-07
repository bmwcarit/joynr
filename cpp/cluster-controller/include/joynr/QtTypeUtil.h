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

#ifndef QTTYPEUTIL
#define QTTYPEUTIL
#include "joynr/JoynrCommonExport.h"

#include <QString>
#include <string>
#include <QList>
#include <QVector>
#include <qglobal.h>
#include <vector>
#include <QByteArray>
#include <stdint.h>
#include <chrono>

#include "joynr/Variant.h"

namespace joynr
{

/**
  * @class QtTypeUtil
  * @brief Container class for helper methods related with the used datatypes
  */
class JOYNRCOMMON_EXPORT QtTypeUtil
{
public:
    /**
      * Converts a vector of std objects into a list of qt objects
      */
    template <class STDTYPE, class QTTYPE, class QTTYPECONVERTER>
    static QList<QTTYPE> toQt(const std::vector<STDTYPE>& stdValues)
    {
        QList<QTTYPE> qtValues;

        for (STDTYPE stdValue : stdValues) {
            qtValues.append(QTTYPECONVERTER::createQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a vector of std objects into a list of qt objects
      */
    template <class STDTYPE, class QTTYPE>
    static QList<QTTYPE> toQt(const std::vector<STDTYPE>& stdValues)
    {
        QList<QTTYPE> qtValues;

        for (STDTYPE stdValue : stdValues) {
            qtValues.append(QTTYPE::createQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a vector of qt objects into a list of qt objects
      */
    template <class QTTYPE>
    static QList<QTTYPE> toQt(const std::vector<QTTYPE>& stdValues)
    {
        QList<QTTYPE> qtValues;

        for (QTTYPE stdValue : stdValues) {
            qtValues.append(stdValue);
        }

        return qtValues;
    }

    /**
      * Converts a list of qt objects into a vector of std objects
      */
    template <class QTTYPE, class STDTYPE, class QTTYPECONVERTER>
    static std::vector<STDTYPE> toStd(const QList<QTTYPE>& qtValues)
    {
        std::vector<STDTYPE> stdValues;

        for (QTTYPE qtValue : qtValues) {
            stdValues.push_back(QTTYPECONVERTER::createStd(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of qt objects into a vector of std objects
      */
    template <class QTTYPE, class STDTYPE>
    static std::vector<STDTYPE> toStd(const QList<QTTYPE>& qtValues)
    {
        std::vector<STDTYPE> stdValues;

        for (QTTYPE qtValue : qtValues) {
            stdValues.push_back(QTTYPE::createStd(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a list of qt objects into a vector of qt objects
      */
    template <class QTTYPE>
    static std::vector<QTTYPE> toStd(const QList<QTTYPE>& qtValues)
    {
        std::vector<QTTYPE> stdValues;

        for (QTTYPE qtValue : qtValues) {
            stdValues.push_back(qtValue);
        }

        return stdValues;
    }

    /**
      * Converts a QString object into a std::string object
      */
    static std::string toStd(const QString& qtString)
    {
        return qtString.toStdString();
    }

    /**
      * Converts a list of QString objects into a vector of std::string objects
      */
    static std::vector<std::string> toStd(const QList<QString>& qtStrings)
    {
        std::vector<std::string> stdStrings;

        for (QString qtString : qtStrings) {
            stdStrings.push_back(toStd(qtString));
        }

        return stdStrings;
    }

    /**
      * Converts a vector of std::string objects into a list of QString objects
      */
    static QList<QString> toQt(const std::vector<std::string>& stdStrings)
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
      * Converts a list of qint8 values into a vector of int8_t objects
      */
    static std::vector<int8_t> toStdInt8(const QList<qint8>& qtValues)
    {
        std::vector<int8_t> stdValues;

        for (qint8 qtValue : qtValues) {
            stdValues.push_back(toStdInt8(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of int8_t objects into a list of qint8 objects
      */
    static QList<qint8> toQt(const std::vector<int8_t>& stdValues)
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
      * Converts a uint8_t object into a quint8 object
      */
    static quint8 toQt(const uint8_t& stdValue)
    {
        return static_cast<quint8>(stdValue);
    }

    /**
      * Converts a int16_t object into a qint16 object
      */
    static qint16 toQt(const int16_t& stdValue)
    {
        return static_cast<qint16>(stdValue);
    }

    /**
      * Converts a uint16_t object into a quint16 object
      */
    static quint16 toQt(const uint16_t& stdValue)
    {
        return static_cast<quint16>(stdValue);
    }

    /**
      * Converts a uint32_t object into a quint32 object
      */
    static quint32 toQt(const uint32_t& stdValue)
    {
        return static_cast<quint32>(stdValue);
    }

    /**
      * Converts a qint8 into a uint8_t
      */
    static uint8_t toStdUInt8(const qint8& qtValue)
    {
        return static_cast<uint8_t>(qtValue);
    }

    /**
      * Converts a list of qint8 values into a vector of uint8_t objects
      */
    static std::vector<uint8_t> toStdUInt8(const QList<qint8>& qtValues)
    {
        std::vector<uint8_t> stdValues;

        for (qint8 qtValue : qtValues) {
            stdValues.push_back(toStdUInt8(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of uint8_t objects into a list of qint8 objects
      */
    static QList<qint8> toQt(const std::vector<uint8_t>& stdValues)
    {
        QList<qint8> qtValues;

        for (uint8_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a list of int values into a vector of int16_t objects
      */
    static std::vector<int16_t> toStdInt16(const QList<int>& qtValues)
    {
        std::vector<int16_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(static_cast<int16_t>(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of int16_t objects into a list of int objects
      */
    static QList<int> toQt(const std::vector<int16_t>& stdValues)
    {
        QList<int> qtValues;

        for (int16_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a list of int values into a vector of uint16_t objects
      */
    static std::vector<uint16_t> toStdUInt16(const QList<int>& qtValues)
    {
        std::vector<uint16_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(static_cast<uint16_t>(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of uint16_t objects into a list of int objects
      */
    static QList<int> toQt(const std::vector<uint16_t>& stdValues)
    {
        QList<int> qtValues;

        for (uint16_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a list of int values into a vector of int32_t objects
      */
    static std::vector<int32_t> toStdInt32(const QList<int>& qtValues)
    {
        std::vector<int32_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(static_cast<int32_t>(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of int32_t objects into a list of int objects
      */
    static QList<int> toQt(const std::vector<int32_t>& stdValues)
    {
        QList<int> qtValues;

        for (int32_t stdValue : stdValues) {
            qtValues.append(static_cast<int>(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a list of int values into a vector of uint32_t objects
      */
    static std::vector<uint32_t> toStdUInt32(const QList<int>& qtValues)
    {
        std::vector<uint32_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(static_cast<uint32_t>(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of uint32_t objects into a list of int objects
      */
    static QList<int> toQt(const std::vector<uint32_t>& stdValues)
    {
        QList<int> qtValues;

        for (uint32_t stdValue : stdValues) {
            qtValues.append(static_cast<int>(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a list of int64_t values into a vector of int64_t objects
      */
    static std::vector<int64_t> toStdInt64(const QList<int64_t>& qtValues)
    {
        QVector<int64_t> qtVector = qtValues.toVector();
        return qtVector.toStdVector();
    }

    /**
      * Converts a vector of int64_t objects into a list of int64_t objects
      */
    static QList<int64_t> toQt(const std::vector<int64_t>& stdValues)
    {
        QList<int64_t> qtValues;

        for (int64_t stdValue : stdValues) {
            qtValues.append(stdValue);
        }

        return qtValues;
    }

    /**
      * Converts a list of int64_t values into a vector of uint64_t objects
      */
    static std::vector<uint64_t> toStdUInt64(const QList<int64_t>& qtValues)
    {
        std::vector<uint64_t> stdValues;

        for (int64_t value : qtValues) {
            stdValues.push_back(static_cast<uint64_t>(value));
        }

        return stdValues;
    }

    /**
      * Converts a vector of uint64_t objects into a list of int64_t objects
      */
    static QList<int64_t> toQt(const std::vector<uint64_t>& stdValues)
    {
        QList<int64_t> qtValues;

        for (uint64_t stdValue : stdValues) {
            qtValues.append(static_cast<int64_t>(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a list of double values into a vector of float objects
      */
    static std::vector<float> toStdFloat(const QList<double>& qtValues)
    {
        std::vector<float> stdValues;

        for (double qtValue : qtValues) {
            stdValues.push_back(static_cast<float>(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of float objects into a list of double objects
      */
    static QList<double> toQt(const std::vector<float>& stdValues)
    {
        QList<double> qtValues;

        for (float stdValue : stdValues) {
            qtValues.append(static_cast<double>(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a QByteArray into a std::vector<uint8_t>
      */
    static std::vector<uint8_t> toStd(const QByteArray& qtValue)
    {
        std::vector<uint8_t> stdValue(qtValue.begin(), qtValue.end());

        return stdValue;
    }

    /**
      * Converts a vector of std::vector<uint8_t> objects into a list of QByteArray objects
      */
    static QList<QByteArray> toQt(const std::vector<std::vector<uint8_t>>& stdValues)
    {
        QList<QByteArray> qtValues;

        for (std::vector<uint8_t> stdValue : stdValues) {
            qtValues.append(toQByteArray(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a std::vector<uint8_t> object into a QByteArray object
      */
    static QByteArray toQByteArray(const std::vector<uint8_t>& stdValue)
    {
        QByteArray qtValue;
        for (const uint8_t entry : stdValue) {
            qtValue.append(entry);
        }
        return qtValue;
    }
};
} // namespace joynr
#endif // QTTYPEUTIL
