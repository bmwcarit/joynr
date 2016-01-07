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

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

#include "joynr/Variant.h"

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
      * Converts a int into a std::int16_t
      */
    static std::int16_t toStdInt16(const int& qtValue)
    {
        return static_cast<std::int16_t>(qtValue);
    }

    /**
      * Converts a std::int16_t object into a int object
      */
    static int toInt(const std::int16_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a int into a std::uint16_t
      */
    static std::uint16_t toStdUInt16(const int& qtValue)
    {
        return static_cast<std::uint16_t>(qtValue);
    }

    /**
      * Converts a std::uint16_t object into a int object
      */
    static int toInt(const std::uint16_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a std::int32_t object into a int object
      */
    static int toInt(const std::int32_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a std::uint32_t object into a int object
      */
    static int toInt(const std::uint32_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a int into a std::int32_t
      */
    static std::int32_t toStdInt32(const int& qtValue)
    {
        return static_cast<std::int32_t>(qtValue);
    }

    /**
      * Converts a int32_t object into a int object
      */
    static int toQt(const std::int32_t& stdValue)
    {
        return static_cast<int>(stdValue);
    }

    /**
      * Converts a int into a std::uint32_t
      */
    static std::uint32_t toStdUInt32(const int& qtValue)
    {
        return static_cast<std::uint32_t>(qtValue);
    }

    /**
      * Converts a std::int64_t into a std::int64_t
      */
    static std::int64_t toStdInt64(const std::int64_t& qtValue)
    {
        return static_cast<std::int64_t>(qtValue);
    }

    /**
      * Converts a std::uint64_t object into a std::int64_t object
      */
    static std::int64_t toStdInt64(const std::uint64_t& stdValue)
    {
        return static_cast<std::int64_t>(stdValue);
    }

    /**
      * Converts a std::int64_t into a std::uint64_t
      */
    static std::uint64_t toStdUInt64(const std::int64_t& qtValue)
    {
        return static_cast<std::uint64_t>(qtValue);
    }

    /**
      * Converts a std::uint64_t object into a std::int64_t object
      */
    static std::int64_t toQt(const std::uint64_t& stdValue)
    {
        return static_cast<std::int64_t>(stdValue);
    }

    /**
      * Converts a double into a float
      */
    static float toStdFloat(const double& qtValue)
    {
        return static_cast<float>(qtValue);
    }

    /**
      * Converts a float object into a double object
      */
    static double toDouble(const float& stdValue)
    {
        return static_cast<double>(stdValue);
    }

    /**
      * Converts a std::chrono::system_clock::time_point to milliseconds
      */
    static std::uint64_t toMilliseconds(const std::chrono::system_clock::time_point& timePoint)
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
#endif // TYPE_UTIL_H_
