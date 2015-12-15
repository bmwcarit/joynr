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
#include <vector>
#include <QByteArray>
#include <stdint.h>
#include <chrono>

#include "joynr/Variant.h"

using namespace std::chrono;

namespace joynr
{

/**
  * @class TypeUtil
  * @brief Container class for helper methods related with the used datatypes
  */
class JOYNRCOMMON_EXPORT TypeUtil
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
      * Converts a list of int values into a vector of int16_t objects
      */
    static std::vector<int16_t> toStdInt16(const QList<int>& qtValues)
    {
        std::vector<int16_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(toStdInt16(qtValue));
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
      * Converts a list of int values into a vector of uint16_t objects
      */
    static std::vector<uint16_t> toStdUInt16(const QList<int>& qtValues)
    {
        std::vector<uint16_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(toStdUInt16(qtValue));
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
      * Converts a list of int values into a vector of int32_t objects
      */
    static std::vector<int32_t> toStdInt32(const QList<int>& qtValues)
    {
        std::vector<int32_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(toStdInt32(qtValue));
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
      * Converts a list of int values into a vector of uint32_t objects
      */
    static std::vector<uint32_t> toStdUInt32(const QList<int>& qtValues)
    {
        std::vector<uint32_t> stdValues;

        for (int qtValue : qtValues) {
            stdValues.push_back(toStdUInt32(qtValue));
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

    /**
      * Converts a qint64 into a int64_t
      */
    static int64_t toStdInt64(const qint64& qtValue)
    {
        return static_cast<int64_t>(qtValue);
    }

    /**
      * Converts a list of qint64 values into a vector of int64_t objects
      */
    static std::vector<int64_t> toStdInt64(const QList<qint64>& qtValues)
    {
        std::vector<int64_t> stdValues;

        for (qint64 qtValue : qtValues) {
            stdValues.push_back(toStdInt64(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of int64_t objects into a list of qint64 objects
      */
    static QList<qint64> toQt(const std::vector<int64_t>& stdValues)
    {
        QList<qint64> qtValues;

        for (int64_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a int64_t object into a qint64 object
      */
    static qint64 toQt(const int64_t& stdValue)
    {
        return static_cast<qint64>(stdValue);
    }

    /**
      * Converts a qint64 into a uint64_t
      */
    static uint64_t toStdUInt64(const qint64& qtValue)
    {
        return static_cast<uint64_t>(qtValue);
    }

    /**
      * Converts a list of qint64 values into a vector of uint64_t objects
      */
    static std::vector<uint64_t> toStdUInt64(const QList<qint64>& qtValues)
    {
        std::vector<uint64_t> stdValues;

        for (qint64 qtValue : qtValues) {
            stdValues.push_back(toStdUInt64(qtValue));
        }

        return stdValues;
    }

    /**
      * Converts a vector of uint64_t objects into a list of qint64 objects
      */
    static QList<qint64> toQt(const std::vector<uint64_t>& stdValues)
    {
        QList<qint64> qtValues;

        for (uint64_t stdValue : stdValues) {
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a uint64_t object into a qint64 object
      */
    static qint64 toQt(const uint64_t& stdValue)
    {
        return static_cast<qint64>(stdValue);
    }

    /**
      * Converts a double into a float
      */
    static float toStdFloat(const double& qtValue)
    {
        return static_cast<float>(qtValue);
    }

    /**
      * Converts a list of double values into a vector of float objects
      */
    static std::vector<float> toStdFloat(const QList<double>& qtValues)
    {
        std::vector<float> stdValues;

        for (double qtValue : qtValues) {
            stdValues.push_back(toStdFloat(qtValue));
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
            qtValues.append(toQt(stdValue));
        }

        return qtValues;
    }

    /**
      * Converts a float object into a double object
      */
    static double toQt(const float& stdValue)
    {
        return static_cast<double>(stdValue);
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
      * Converts a list of QByteArray values into a vector of std::vector<uint8_t> objects
      */
    static std::vector<std::vector<uint8_t>> toStd(const QList<QByteArray>& qtValues)
    {
        std::vector<std::vector<uint8_t>> stdValues;

        for (QByteArray qtValue : qtValues) {
            stdValues.push_back(toStd(qtValue));
        }

        return stdValues;
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

    /**
      * Converts a std::chrono::system_clock::time_point to milliseconds
      */
    static uint64_t toMilliseconds(const std::chrono::system_clock::time_point& timePoint)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch())
                .count();
    }

    /**
      * Converts a std::chrono::system_clock::time_point to a printable string
      */
    static std::string toDateString(const std::chrono::system_clock::time_point& timePoint)
    {
        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        return std::ctime(&time);
    }

    /**
      * Converts a vector of typename T objects into a vector of Variant objects
      */
    template <typename T>
    static std::vector<Variant> toVectorOfVariants(const std::vector<T>& values)
    {
        std::vector<Variant> variantValues;
        variantValues.reserve(values.size());

        for (const T& value : values) {
            variantValues.push_back(Variant::make<T>(value));
        }

        return variantValues;
    }

    /**
      * Converts a vector of typename T objects into a Variant object
      */
    template <typename T>
    static Variant toVariant(const std::vector<T>& values)
    {
        std::vector<Variant> variantValues;
        variantValues.reserve(values.size());

        for (const T& value : values) {
            variantValues.push_back(Variant::make<T>(value));
        }

        return toVariant(variantValues);
    }

    /**
      * Converts a vector of Variant objects into a Variant object
      */
    static Variant toVariant(const std::vector<Variant>& values)
    {
        return Variant::make<std::vector<Variant>>(values);
    }
};
} // namespace joynr
#endif /* TYPE_UTIL_H_ */
