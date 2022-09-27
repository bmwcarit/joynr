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

#ifndef ILTUTIL_H
#define ILTUTIL_H
#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/interlanguagetest/Enumeration.h"
#include "joynr/interlanguagetest/EnumerationWithoutDefinedValues.h"
#include "joynr/interlanguagetest/TestInterfaceAbstractProvider.h"
#include "joynr/interlanguagetest/namedTypeCollection1/StructWithStringArray.h"
#include "joynr/interlanguagetest/namedTypeCollection2/BaseStruct.h"
#include "joynr/interlanguagetest/namedTypeCollection2/BaseStructWithoutElements.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedBaseStruct.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedEnumerationWithPartlyDefinedValues.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedExtendedBaseStruct.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedExtendedEnumeration.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedInterfaceEnumerationInTypeCollection.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedStructOfPrimitives.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.h"
#include "math.h"
#include <functional>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

using namespace joynr;

class IltUtil
{
public:
    // no constructor

    static bool useRestricted64BitRange;
    static bool useRestrictedUnsignedRange;
    ADD_LOGGER(IltUtil)

    // static methods

    // all check* methods return true, if check passed OK, false otherwise.
    // all fill* methods expect reference to existing object; also return that object
    // all create* methods return new object instance or primitive value

    // StringArray
    static std::vector<std::string> fillStringArray(std::vector<std::string>& stringArray)
    {
        stringArray.clear();
        stringArray.push_back("Hello");
        stringArray.push_back("World");
        if (!checkStringArray(stringArray)) {
            throw new joynr::exceptions::JoynrRuntimeException("Internal error in fillStringArray");
        }
        return stringArray;
    }

    static std::vector<std::string> createStringArray(void)
    {
        std::vector<std::string> stringArray;
        fillStringArray(stringArray);
        return stringArray;
    }

    static bool checkStringArray(const std::vector<std::string>& stringArray)
    {
        if (stringArray.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkStringArray: array has invalid size");
            return false;
        }
        if (stringArray.at(0) != "Hello" || stringArray.at(1) != "World") {
            JOYNR_LOG_TRACE(logger(), "checkStringArray: invalid content found");
            return false;
        }
        return true;
    }

    static bool checkStringArray(const std::vector<std::string*>& stringArray)
    {
        if (stringArray.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkStringArray: array has invalid size");
            return false;
        }
        if (*stringArray.at(0) != "Hello" || *stringArray.at(1) != "World") {
            JOYNR_LOG_TRACE(logger(), "checkStringArray: invalid content found");
            return false;
        }
        return true;
    }

    // Byte Array
    static std::vector<int8_t> fillByteArray(std::vector<int8_t>& byteArray)
    {
        byteArray.clear();
        byteArray.push_back(1);
        byteArray.push_back(127);
        if (!checkByteArray(byteArray)) {
            throw new joynr::exceptions::JoynrRuntimeException("Internal error in fillByteArray");
        }
        return byteArray;
    }

    static std::vector<int8_t> createByteArray(void)
    {
        std::vector<int8_t> byteArray;
        fillByteArray(byteArray);
        return byteArray;
    }

    static bool checkByteArray(const std::vector<int8_t>& byteArray)
    {
        if (byteArray.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkByteArray: invalid array size");
            return false;
        }
        if (byteArray.at(0) != 1 || byteArray.at(1) != 127) {
            JOYNR_LOG_TRACE(logger(), "checkByteArray: invalid content found");
            return false;
        }
        return true;
    }

    static std::vector<std::uint64_t> convertInt8ArrayToUInt64Array(
            const std::vector<std::int8_t>& int8Array)
    {
        std::vector<std::uint64_t> uInt64Array;
        for (auto it = int8Array.cbegin(); it != int8Array.cend(); it++) {
            // TODO: Beware, input could be negative, maybe increase by +128 to make it positive?
            uInt64Array.push_back(*it);
        }
        return uInt64Array;
    }

    // uint64_t Array
    static std::vector<uint64_t> fillUInt64Array(std::vector<uint64_t>& uInt64Array)
    {
        uInt64Array.clear();
        uInt64Array.push_back(1);
        uInt64Array.push_back(127);
        if (!checkUInt64Array(uInt64Array)) {
            throw new joynr::exceptions::JoynrRuntimeException("Internal error in fillUInt64Array");
        }
        return uInt64Array;
    }

    static std::vector<uint64_t> createUInt64Array(void)
    {
        std::vector<uint64_t> uInt64Array;
        fillUInt64Array(uInt64Array);
        return uInt64Array;
    }

    static bool checkUInt64Array(const std::vector<uint64_t>& uInt64Array)
    {
        if (uInt64Array.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkUInt64Array: array has invalid size");
            return false;
        }
        if (uInt64Array.at(0) != 1 || uInt64Array.at(1) != 127) {
            JOYNR_LOG_TRACE(logger(), "checkUInt64Array: array has invalid content");
            return false;
        }
        return true;
    }

    // Double Array
    static std::vector<double> fillDoubleArray(std::vector<double>& doubleArray)
    {
        doubleArray.clear();
        doubleArray.push_back(1.1);
        doubleArray.push_back(2.2);
        doubleArray.push_back(3.3);
        if (!checkDoubleArray(doubleArray)) {
            throw new joynr::exceptions::JoynrRuntimeException("Internal error in fillDoubleArray");
        }
        return doubleArray;
    }

    static std::vector<double> createDoubleArray(void)
    {
        std::vector<double> doubleArray;
        fillDoubleArray(doubleArray);
        return doubleArray;
    }

    static bool checkDoubleArray(const std::vector<double>& doubleArray)
    {
        if (doubleArray.size() != 3) {
            JOYNR_LOG_TRACE(logger(), "checkDoubleArray: array has invalid size");
            return false;
        }
        if (!cmpDouble(doubleArray.at(0), 1.1) || !cmpDouble(doubleArray.at(1), 2.2) ||
            !cmpDouble(doubleArray.at(2), 3.3)) {
            JOYNR_LOG_TRACE(logger(), "checkDoubleArray: array has invalid content");
            return false;
        }
        return true;
    }

    // ExtendedInterfaceEnumerationInTypeCollectionArray
    static std::vector<joynr::interlanguagetest::namedTypeCollection2::
                               ExtendedInterfaceEnumerationInTypeCollection::Enum>
    fillExtendedInterfaceEnumerationInTypeCollectionArray(
            std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                ExtendedInterfaceEnumerationInTypeCollection::Enum>&
                    extendedInterfaceEnumerationInTypeCollection)
    {
        extendedInterfaceEnumerationInTypeCollection.clear();
        extendedInterfaceEnumerationInTypeCollection.push_back(
                joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedInterfaceEnumerationInTypeCollection::Enum::
                                ENUM_2_VALUE_EXTENSION_FOR_INTERFACE);
        extendedInterfaceEnumerationInTypeCollection.push_back(
                joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedInterfaceEnumerationInTypeCollection::Enum::ENUM_I1_VALUE_3);
        if (!checkExtendedInterfaceEnumerationInTypeCollectionArray(
                    extendedInterfaceEnumerationInTypeCollection)) {
            throw new joynr::exceptions::JoynrRuntimeException(
                    "Internal error in fillExtendedInterfaceEnumerationInTypeCollectionArray");
        }
        return extendedInterfaceEnumerationInTypeCollection;
    }

    static std::vector<joynr::interlanguagetest::namedTypeCollection2::
                               ExtendedInterfaceEnumerationInTypeCollection::Enum>
    createExtendedInterfaceEnumerationInTypeCollectionArray(void)
    {
        std::vector<joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedInterfaceEnumerationInTypeCollection::Enum>
                extendedInterfaceEnumerationInTypeCollection;
        fillExtendedInterfaceEnumerationInTypeCollectionArray(
                extendedInterfaceEnumerationInTypeCollection);
        return extendedInterfaceEnumerationInTypeCollection;
    }

    static bool checkExtendedInterfaceEnumerationInTypeCollectionArray(
            const std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedInterfaceEnumerationInTypeCollection::Enum>&
                    extendedInterfaceEnumerationInTypeCollection)
    {
        if (extendedInterfaceEnumerationInTypeCollection.size() != 2) {
            JOYNR_LOG_TRACE(logger(),
                            "checkExtendedInterfaceEnumerationInTypeCollectionArray: array has "
                            "invalid size");
            return false;
        }
        if (extendedInterfaceEnumerationInTypeCollection.at(0) !=
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedInterfaceEnumerationInTypeCollection::Enum::
                                    ENUM_2_VALUE_EXTENSION_FOR_INTERFACE ||
            extendedInterfaceEnumerationInTypeCollection.at(1) !=
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedInterfaceEnumerationInTypeCollection::Enum::ENUM_I1_VALUE_3) {
            JOYNR_LOG_TRACE(logger(),
                            "checkExtendedInterfaceEnumerationInTypeCollectionArray: array has "
                            "invalid content");
            return false;
        }
        return true;
    }

    // ExtendedExtendedEnumerationArray
    static std::vector<
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>
    fillExtendedExtendedEnumerationArray(
            std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                ExtendedExtendedEnumeration::Enum>&
                    extendedExtendedEnumerationArray)
    {
        extendedExtendedEnumerationArray.clear();
        extendedExtendedEnumerationArray.push_back(
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum::
                        ENUM_2_VALUE_EXTENSION_EXTENDED);
        extendedExtendedEnumerationArray.push_back(
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum::
                        ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
        return extendedExtendedEnumerationArray;
    }

    static std::vector<
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>
    createExtendedExtendedEnumerationArray(void)
    {
        std::vector<
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>
                extendedExtendedEnumerationArray;
        fillExtendedExtendedEnumerationArray(extendedExtendedEnumerationArray);
        return extendedExtendedEnumerationArray;
    }

    static bool checkExtendedExtendedEnumerationArray(
            const std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedExtendedEnumeration::Enum>&
                    extendedExtendedEnumerationArray)
    {
        if (extendedExtendedEnumerationArray.size() != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkExtendedExtendedEnumerationArray: array has invalid size");
            return false;
        }
        if (extendedExtendedEnumerationArray.at(0) !=
                    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::
                            Enum::ENUM_2_VALUE_EXTENSION_EXTENDED ||
            extendedExtendedEnumerationArray.at(1) !=
                    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::
                            Enum::ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_TRACE(
                    logger(), "checkExtendedExtendedEnumerationArray: array has invalid content");
            return false;
        }
        return true;
    }

    // StructWithStringArray
    static joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray
    fillStructWithStringArray(joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                                      structWithStringArray)
    {
        std::vector<std::string> stringArray;
        stringArray.clear();
        stringArray.push_back("Hello");
        stringArray.push_back("World");
        structWithStringArray.setStringArrayElement(stringArray);
        return structWithStringArray;
    }

    static joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray
    createStructWithStringArray(void)
    {
        joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray structWithStringArray;
        fillStructWithStringArray(structWithStringArray);
        return structWithStringArray;
    }

    static bool checkStructWithStringArray(
            const joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                    structWithStringArray)
    {
        std::vector<std::string> stringArray = structWithStringArray.getStringArrayElement();
        if (stringArray.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkStructWithStringArray: array size != 2");
            return false;
        }
        if (stringArray.at(0) != "Hello" || stringArray.at(1) != "World") {
            JOYNR_LOG_TRACE(logger(), "checkStructWithStringArray: invalid content found");
            return false;
        }
        return true;
    }

    // StructWithStringArrayArray
    static std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
    fillStructWithStringArrayArray(
            std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                    structWithStringArrayArray)
    {
        structWithStringArrayArray.clear();
        structWithStringArrayArray.push_back(createStructWithStringArray());
        structWithStringArrayArray.push_back(createStructWithStringArray());
        if (!checkStructWithStringArrayArray(structWithStringArrayArray)) {
            throw new joynr::exceptions::JoynrRuntimeException(
                    "Internal error in fillStructWithStringArrayArray");
        }
        return structWithStringArrayArray;
    }

    static std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
    createStructWithStringArrayArray(void)
    {
        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                structWithStringArrayArray;
        fillStructWithStringArrayArray(structWithStringArrayArray);
        return structWithStringArrayArray;
    }

    static bool checkStructWithStringArrayArray(
            const std::vector<
                    joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                    structWithStringArrayArray)
    {
        if (structWithStringArrayArray.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkStructWithStringArrayArray: size != 2");
            return false;
        }
        if (!checkStructWithStringArray(structWithStringArrayArray.at(0)) ||
            !checkStructWithStringArray(structWithStringArrayArray.at(1))) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructWithStringArrayArray: checkStructWithStringArray fails");
            return false;
        }
        return true;
    }

    static bool checkStructWithStringArrayArray(
            const std::vector<
                    joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray*>&
                    structWithStringArrayArray)
    {
        if (structWithStringArrayArray.size() != 2) {
            JOYNR_LOG_TRACE(logger(), "checkStructWithStringArrayArray: size != 2");
            return false;
        }
        if (!checkStructWithStringArray(*structWithStringArrayArray.at(0)) ||
            !checkStructWithStringArray(*structWithStringArrayArray.at(1))) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructWithStringArrayArray: checkStructWithStringArray fails");
            return false;
        }
        return true;
    }

    // BaseStructWithoutElements
    static joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
    createBaseStructWithoutElements(void)
    {
        joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
                baseStructWithoutElements;
        // empty struct
        return baseStructWithoutElements;
    }

    static bool checkBaseStructWithoutElements(
            const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                    baseStructWithoutElements)
    {
        // empty struct
        return true;
    }

    // BaseStruct
    static joynr::interlanguagetest::namedTypeCollection2::BaseStruct fillBaseStruct(
            joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStruct)
    {
        baseStruct.setBaseStructString("Hiya");
        if (!checkBaseStruct(baseStruct)) {
            throw new joynr::exceptions::JoynrRuntimeException("Internal error in fillBaseStruct");
        }
        return baseStruct;
    }

    static joynr::interlanguagetest::namedTypeCollection2::BaseStruct createBaseStruct(void)
    {
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStruct;
        fillBaseStruct(baseStruct);
        return baseStruct;
    }

    static bool checkBaseStruct(
            const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStruct)
    {
        if (baseStruct.getBaseStructString() != "Hiya") {
            JOYNR_LOG_TRACE(logger(), "checkBaseStruct: baseStructString has invalid content");
            return false;
        }
        return true;
    }

    // ExtendedBaseStruct
    static joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct
    fillExtendedBaseStruct(
            joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct& extendedBaseStruct)
    {
        extendedBaseStruct.setEnumElement(joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_3);
        fillBaseStruct(extendedBaseStruct);
        return extendedBaseStruct;
    }

    static joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct
    createExtendedBaseStruct(void)
    {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStruct;
        fillExtendedBaseStruct(extendedBaseStruct);
        return extendedBaseStruct;
    }

    static bool checkExtendedBaseStruct(
            const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                    extendedBaseStruct)
    {
        if (extendedBaseStruct.getEnumElement() !=
            joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_3) {
            JOYNR_LOG_TRACE(logger(), "checkExtendedBaseStruct: enumElement has invalid content");
            return false;
        }
        // check inherited parts
        if (!checkBaseStruct(extendedBaseStruct)) {
            JOYNR_LOG_TRACE(logger(), "checkExtendedBaseStruct: baseStruct has invalid content");
            return false;
        }
        return true;
    }

    // ExtendedExtendedBaseStruct
    static joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
    fillExtendedExtendedBaseStruct(
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                    extendedExtendedBaseStruct)
    {
        extendedExtendedBaseStruct.setEnumWithoutDefinedValuesElement(
                joynr::interlanguagetest::EnumerationWithoutDefinedValues::ENUM_0_VALUE_1);
        fillExtendedBaseStruct(extendedExtendedBaseStruct);
        if (!checkExtendedExtendedBaseStruct(extendedExtendedBaseStruct)) {
            throw new joynr::exceptions::JoynrRuntimeException(
                    "Internal error in fillExtendedExtendedBaseStruct");
        }
        return extendedExtendedBaseStruct;
    }

    static joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
    createExtendedExtendedBaseStruct(void)
    {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
                extendedExtendedBaseStruct;
        fillExtendedExtendedBaseStruct(extendedExtendedBaseStruct);
        return extendedExtendedBaseStruct;
    }

    static bool checkExtendedExtendedBaseStruct(
            const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                    extendedExtendedBaseStruct)
    {
        if (extendedExtendedBaseStruct.getEnumWithoutDefinedValuesElement() !=
            joynr::interlanguagetest::EnumerationWithoutDefinedValues::ENUM_0_VALUE_1) {
            JOYNR_LOG_TRACE(logger(),
                            "checkExtendedExtendedBaseStruct: enumWithoutDefinedValuesElement has "
                            "invalid content");
            return false;
        }
        // check inherited parts
        if (!checkExtendedBaseStruct(extendedExtendedBaseStruct)) {
            JOYNR_LOG_TRACE(
                    logger(),
                    "checkExtendedExtendedBaseStruct: extendedBaseStruct has invalid content");
            return false;
        }
        return true;
    }

    // StructOfPrimitives
    static joynr::interlanguagetest::namedTypeCollection2::StructOfPrimitives
    fillStructOfPrimitives(
            joynr::interlanguagetest::namedTypeCollection2::StructOfPrimitives& structOfPrimitives)
    {
        structOfPrimitives.setBooleanElement(true);
        structOfPrimitives.setDoubleElement(1.1);
        structOfPrimitives.setFloatElement(1.1f);
        structOfPrimitives.setInt8MinElement(SCHAR_MIN);
        structOfPrimitives.setInt8MaxElement(SCHAR_MAX);
        structOfPrimitives.setInt16MinElement(SHRT_MIN);
        structOfPrimitives.setInt16MaxElement(SHRT_MAX);
        structOfPrimitives.setInt32MinElement(INT_MIN);
        structOfPrimitives.setInt32MaxElement(INT_MAX);

        //
        // Use limited value range for int64 testing
        // in order to avoid violation of maximum safe
        // integer range of Javascript
        if (useRestricted64BitRange) {
            structOfPrimitives.setInt64MinElement(-9007199254740991L);
            structOfPrimitives.setInt64MaxElement(9007199254740991L);
        } else {
            structOfPrimitives.setInt64MinElement(LLONG_MIN);
            structOfPrimitives.setInt64MaxElement(LLONG_MAX);
        }

        structOfPrimitives.setConstString("Hiya");

        structOfPrimitives.setUInt8MinElement(0);
        if (useRestrictedUnsignedRange) {
            // Java, Javascript and C++ compatible
            structOfPrimitives.setUInt8MaxElement(CHAR_MAX);
        } else {
            // Javascript, C++ compatible
            structOfPrimitives.setUInt8MaxElement(UCHAR_MAX);
        }

        structOfPrimitives.setUInt16MinElement(0);
        if (useRestrictedUnsignedRange) {
            // Java, Javascript and C++ compatible
            structOfPrimitives.setUInt16MaxElement(SHRT_MAX);
        } else {
            // Javascript, C++ compatible
            structOfPrimitives.setUInt16MaxElement(USHRT_MAX);
        }

        structOfPrimitives.setUInt32MinElement(0);
        if (useRestrictedUnsignedRange) {
            // Java, Javascript and C++ compatible
            structOfPrimitives.setUInt32MaxElement(INT_MAX);
        } else {
            // Javascript, C++ compatible
            structOfPrimitives.setUInt32MaxElement(4294967295U);
        }

        structOfPrimitives.setUInt64MinElement(0L);
        if (useRestricted64BitRange) {
            // Java, Javascript and C++ compatible
            // use maximum safe integer range.
            structOfPrimitives.setUInt64MaxElement(9007199254740991ULL);
        } else if (useRestrictedUnsignedRange) {
            // Java and C++ compatible
            structOfPrimitives.setUInt64MaxElement(9223372036854775807ULL);
        } else {
            // C++ only
            structOfPrimitives.setUInt64MaxElement(18446744073709551615ULL);
        }

        std::vector<bool> bList;
        bList.clear();
        bList.push_back(true);
        bList.push_back(false);
        structOfPrimitives.setBooleanArray(bList);

        std::vector<double> dList;
        dList.clear();
        dList.push_back(1.1);
        dList.push_back(2.2);
        structOfPrimitives.setDoubleArray(dList);

        std::vector<float> fList;
        fList.clear();
        fList.push_back(1.1f);
        fList.push_back(2.2f);
        structOfPrimitives.setFloatArray(fList);

        std::vector<int8_t> i8List;
        i8List.clear();
        i8List.push_back(1);
        i8List.push_back(2);
        structOfPrimitives.setInt8Array(i8List);

        std::vector<int16_t> i16List;
        i16List.clear();
        i16List.push_back(1);
        i16List.push_back(2);
        structOfPrimitives.setInt16Array(i16List);

        std::vector<int32_t> i32List;
        i32List.clear();
        i32List.push_back(1);
        i32List.push_back(2);
        structOfPrimitives.setInt32Array(i32List);

        std::vector<int64_t> i64List;
        i64List.clear();
        i64List.push_back(1L);
        i64List.push_back(2L);
        structOfPrimitives.setInt64Array(i64List);

        std::vector<std::string> stringList;
        stringList.clear();
        stringList.push_back("Hello");
        stringList.push_back("World");
        structOfPrimitives.setStringArray(stringList);

        std::vector<uint8_t> ui8List;
        ui8List.clear();
        ui8List.push_back(1);
        ui8List.push_back(2);
        structOfPrimitives.setUInt8Array(ui8List);

        std::vector<uint16_t> ui16List;
        ui16List.clear();
        ui16List.push_back(1);
        ui16List.push_back(2);
        structOfPrimitives.setUInt16Array(ui16List);

        std::vector<uint32_t> ui32List;
        ui32List.clear();
        ui32List.push_back(1);
        ui32List.push_back(2);
        structOfPrimitives.setUInt32Array(ui32List);

        std::vector<uint64_t> ui64List;
        ui64List.clear();
        ui64List.push_back(1L);
        ui64List.push_back(2L);
        structOfPrimitives.setUInt64Array(ui64List);

        return structOfPrimitives;
    }

    static joynr::interlanguagetest::namedTypeCollection2::StructOfPrimitives
    createStructOfPrimitives(void)
    {
        joynr::interlanguagetest::namedTypeCollection2::StructOfPrimitives structOfPrimitives;
        fillStructOfPrimitives(structOfPrimitives);
        return structOfPrimitives;
    }

    static bool checkStructOfPrimitives(
            const joynr::interlanguagetest::namedTypeCollection2::StructOfPrimitives&
                    structOfPrimitives)
    {
        JOYNR_LOG_TRACE(logger(), "DUMP checkStructOfPrimitives");
        JOYNR_LOG_TRACE(logger(), "\tbooleanElement {}", structOfPrimitives.getBooleanElement());
        JOYNR_LOG_TRACE(logger(), "\tdoubleElement {}", structOfPrimitives.getDoubleElement());
        JOYNR_LOG_TRACE(logger(), "\tfloatElement {}", structOfPrimitives.getFloatElement());
        JOYNR_LOG_TRACE(
                logger(), "\tint8MinElement {}", (int)structOfPrimitives.getInt8MinElement());
        JOYNR_LOG_TRACE(
                logger(), "\tint8MaxElement {}", (int)structOfPrimitives.getInt8MaxElement());
        JOYNR_LOG_TRACE(logger(), "\tint16MinElement {}", structOfPrimitives.getInt16MinElement());
        JOYNR_LOG_TRACE(logger(), "\tint16MaxElement {}", structOfPrimitives.getInt16MaxElement());
        JOYNR_LOG_TRACE(logger(), "\tint32MinElement {}", structOfPrimitives.getInt32MinElement());
        JOYNR_LOG_TRACE(logger(), "\tint32MaxElement {}", structOfPrimitives.getInt32MaxElement());
        JOYNR_LOG_TRACE(logger(), "\tint64MinElement {}", structOfPrimitives.getInt64MinElement());
        JOYNR_LOG_TRACE(logger(), "\tint64MaxElement {}", structOfPrimitives.getInt64MaxElement());
        JOYNR_LOG_TRACE(logger(),
                        "\tuInt8MinElement {}",
                        (unsigned int)structOfPrimitives.getUInt8MinElement());
        JOYNR_LOG_TRACE(logger(),
                        "\tuInt8MaxElement {}",
                        (unsigned int)structOfPrimitives.getUInt8MaxElement());
        JOYNR_LOG_TRACE(
                logger(), "\tuInt16MinElement {}", structOfPrimitives.getUInt16MinElement());
        JOYNR_LOG_TRACE(
                logger(), "\tuInt16MaxElement {}", structOfPrimitives.getUInt16MaxElement());
        JOYNR_LOG_TRACE(
                logger(), "\tuInt32MinElement {}", structOfPrimitives.getUInt32MinElement());
        JOYNR_LOG_TRACE(
                logger(), "\tuInt32MaxElement {}", structOfPrimitives.getUInt32MaxElement());
        JOYNR_LOG_TRACE(
                logger(), "\tuInt64MinElement {}", structOfPrimitives.getUInt64MinElement());
        JOYNR_LOG_TRACE(
                logger(), "\tuInt64MaxElement {}", structOfPrimitives.getUInt64MaxElement());
        JOYNR_LOG_TRACE(logger(), "END DUMP checkStructOfPrimitives");

        if (structOfPrimitives.getBooleanElement() != true) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter booleanElement");
            return false;
        }

        if (!cmpDouble(structOfPrimitives.getDoubleElement(), 1.1)) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter doubleElement");
            return false;
        }

        if (!cmpFloat(structOfPrimitives.getFloatElement(), 1.1f)) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter floatElement");
            return false;
        }

        if (structOfPrimitives.getInt8MinElement() != SCHAR_MIN) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter int8MinElement");
            return false;
        }

        if (structOfPrimitives.getInt8MaxElement() != SCHAR_MAX) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter int8MaxElement");
            return false;
        }

        if (structOfPrimitives.getInt16MinElement() != SHRT_MIN) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter int16MinElement");
            return false;
        }

        if (structOfPrimitives.getInt16MaxElement() != SHRT_MAX) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter int16MaxElement");
            return false;
        }

        if (structOfPrimitives.getInt32MinElement() != INT_MIN) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter int32MinElement");
            return false;
        }

        if (structOfPrimitives.getInt32MaxElement() != INT_MAX) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter int32MaxElement");
            return false;
        }

        if (useRestricted64BitRange) {
            // Javascript
            //
            // Since the original maximum int64 values would exceed the Javascript
            // maximum safe integer value range, only reduced value range is used.

            if (structOfPrimitives.getInt64MinElement() != -9007199254740991L) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter int64MinElement");
                return false;
            }

            if (structOfPrimitives.getInt64MaxElement() != 9007199254740991L) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter int64MaxElement");
                return false;
            }
        } else {
            // Java, C++
            if (structOfPrimitives.getInt64MinElement() != LLONG_MIN) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter int64MinElement");
                return false;
            }

            if (structOfPrimitives.getInt64MaxElement() != LLONG_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter int64MaxElement");
                return false;
            }
        }

        if (structOfPrimitives.getConstString() != "Hiya") {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter constString");
            return false;
        }

        if (structOfPrimitives.getUInt8MinElement() != 0) {
            JOYNR_LOG_TRACE(logger(), "checkStructOfPrimitives: invalid parameter uint8MinElement");
            return false;
        }

        if (useRestrictedUnsignedRange) {
            // Java
            if (structOfPrimitives.getUInt8MaxElement() != CHAR_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint8MaxElement");
                return false;
            }
        } else {
            // C++, Javascript
            if (structOfPrimitives.getUInt8MaxElement() != UCHAR_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint8MaxElement");
                return false;
            }
        }

        if (structOfPrimitives.getUInt16MinElement() != 0) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter uint16MinElement");
            return false;
        }

        if (useRestrictedUnsignedRange) {
            // Java
            if (structOfPrimitives.getUInt16MaxElement() != SHRT_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint16MaxElement");
                return false;
            }
        } else {
            // C++, Javascript
            if (structOfPrimitives.getUInt16MaxElement() != USHRT_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint16MaxElement");
                return false;
            }
        }

        if (structOfPrimitives.getUInt32MinElement() != 0) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter uint32MinElement");
            return false;
        }

        if (useRestrictedUnsignedRange) {
            // Java
            if (structOfPrimitives.getUInt32MaxElement() != INT_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint32MaxElement");
                return false;
            }
        } else {
            // Javascript, C++
            if (structOfPrimitives.getUInt32MaxElement() != UINT_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint32MaxElement");
                return false;
            }
        }

        if (structOfPrimitives.getUInt64MinElement() != 0L) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter uint64MinElement");
            return false;
        }

        if (useRestricted64BitRange) {
            // Javascript and Java compatible
            if (structOfPrimitives.getUInt64MaxElement() != 9007199254740991ULL) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint64MaxElement");
                return false;
            }
        } else if (useRestrictedUnsignedRange) {
            // Java compatible
            if (structOfPrimitives.getUInt64MaxElement() != LLONG_MAX) {
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint64MaxElement");
                return false;
            }
        } else {
            // C++ compatible
            if (structOfPrimitives.getUInt64MaxElement() !=
                ULLONG_MAX) { // should be 18446744073709551615
                JOYNR_LOG_TRACE(
                        logger(), "checkStructOfPrimitives: invalid parameter uint64MaxElement");
                return false;
            }
        }

        if (structOfPrimitives.getBooleanArray().size() != 2 ||
            structOfPrimitives.getBooleanArray().at(0) != true ||
            structOfPrimitives.getBooleanArray().at(1) != false) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: parameter x: invalid boolean array");
            return false;
        }

        if (structOfPrimitives.getDoubleArray().size() != 2 ||
            !cmpDouble(structOfPrimitives.getDoubleArray().at(0), 1.1) ||
            !cmpDouble(structOfPrimitives.getDoubleArray().at(1), 2.2)) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid double array");
            return false;
        }

        if (structOfPrimitives.getFloatArray().size() != 2 ||
            !cmpFloat(structOfPrimitives.getFloatArray().at(0), 1.1) ||
            !cmpFloat(structOfPrimitives.getFloatArray().at(1), 2.2)) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid float array");
            return false;
        }

        if (structOfPrimitives.getInt16Array().size() != 2 ||
            structOfPrimitives.getInt16Array().at(0) != 1 ||
            structOfPrimitives.getInt16Array().at(1) != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid int16 array");
            return false;
        }

        if (structOfPrimitives.getInt32Array().size() != 2 ||
            structOfPrimitives.getInt32Array().at(0) != 1 ||
            structOfPrimitives.getInt32Array().at(1) != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid int32 array");
            return false;
        }

        if (structOfPrimitives.getInt64Array().size() != 2 ||
            structOfPrimitives.getInt64Array().at(0) != 1L ||
            structOfPrimitives.getInt64Array().at(1) != 2L) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid int64 array");
            return false;
        }

        if (structOfPrimitives.getInt8Array().size() != 2 ||
            structOfPrimitives.getInt8Array().at(0) != 1 ||
            structOfPrimitives.getInt8Array().at(1) != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid int8 array");
            return false;
        }

        if (structOfPrimitives.getStringArray().size() != 2 ||
            structOfPrimitives.getStringArray().at(0) != "Hello" ||
            structOfPrimitives.getStringArray().at(1) != "World") {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid string array");
            return false;
        }

        if (structOfPrimitives.getUInt8Array().size() != 2 ||
            structOfPrimitives.getUInt8Array().at(0) != 1 ||
            structOfPrimitives.getUInt8Array().at(1) != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid uint8 array");
            return false;
        }

        if (structOfPrimitives.getUInt16Array().size() != 2 ||
            structOfPrimitives.getUInt16Array().at(0) != 1 ||
            structOfPrimitives.getUInt16Array().at(1) != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid uint16 array");
            return false;
        }

        if (structOfPrimitives.getUInt32Array().size() != 2 ||
            structOfPrimitives.getUInt32Array().at(0) != 1 ||
            structOfPrimitives.getUInt32Array().at(1) != 2) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid uint32 array");
            return false;
        }

        if (structOfPrimitives.getUInt64Array().size() != 2 ||
            structOfPrimitives.getUInt64Array().at(0) != 1L ||
            structOfPrimitives.getUInt64Array().at(1) != 2L) {
            JOYNR_LOG_TRACE(
                    logger(), "checkStructOfPrimitives: invalid parameter x: invalid uint64 array");
            return false;
        }

        return true;
    }

    // ExtendedStructOfPrimitives
    static joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
    fillExtendedStructOfPrimitives(
            joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                    extendedStructOfPrimitives)
    {
        extendedStructOfPrimitives.setExtendedEnumElement(
                joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedTypeCollectionEnumerationInTypeCollection::
                                ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
        extendedStructOfPrimitives.setExtendedStructElement(createExtendedExtendedBaseStruct());
        fillStructOfPrimitives(extendedStructOfPrimitives);
        if (!checkExtendedStructOfPrimitives(extendedStructOfPrimitives)) {
            throw new joynr::exceptions::JoynrRuntimeException(
                    "Internal error in fillExtendedStructOfPrimitives");
        }
        return extendedStructOfPrimitives;
    }

    static joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
    createExtendedStructOfPrimitives(void)
    {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
                extendedStructOfPrimitives;
        fillExtendedStructOfPrimitives(extendedStructOfPrimitives);
        return extendedStructOfPrimitives;
    }

    static bool checkExtendedStructOfPrimitives(
            const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                    extendedStructOfPrimitives)
    {
        if (extendedStructOfPrimitives.getExtendedEnumElement() !=
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::
                            ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_TRACE(
                    logger(),
                    "checkExtendedStructOfPrimitives: extendedEnumElement has invalid content");
            return false;
        }

        // check inherited parts
        if (!checkExtendedBaseStruct(extendedStructOfPrimitives.getExtendedStructElement())) {
            JOYNR_LOG_TRACE(
                    logger(),
                    "checkExtendedStructOfPrimitives: extendedBaseStruct has invalid content");
            return false;
        }

        if (!checkStructOfPrimitives(extendedStructOfPrimitives)) {
            JOYNR_LOG_TRACE(
                    logger(),
                    "checkExtendedStructOfPrimitives: structOfPrimitives has invalid content");
            return false;
        }
        return true;
    }

    // returns true, if values are nearly equal
    static bool cmpFloat(float a, float b)
    {
        JOYNR_LOG_TRACE(logger(), "cmpFloat: Comparing {} with {}", a, b);
        JOYNR_LOG_TRACE(logger(), "Result: {}", (bool)(fabsf(a - b) < 0.001));
        return fabsf(a - b) < 0.001;
    }

    // returns true, if values are nearly equal
    static bool cmpDouble(double a, double b)
    {
        JOYNR_LOG_TRACE(logger(), "cmpDouble: Comparing {} with {}", a, b);
        JOYNR_LOG_TRACE(logger(), "Result: {}", (bool)(fabs(a - b) < 0.001));
        return fabs(a - b) < 0.001;
    }

    // concatenates the two ByteBuffers in a new ByteBuffer
    static joynr::ByteBuffer concatByteBuffers(const joynr::ByteBuffer& a,
                                               const joynr::ByteBuffer& b)
    {
        joynr::ByteBuffer result = a;
        result.reserve(a.size() + b.size());
        for (auto i = 0; i < b.size(); ++i) {
            result.push_back(b.at(i));
        }
        return result;
    }

    // compares the types of two given parameters
    template <typename T, typename U>
    static bool checkType(T a, U b)
    {
        if (typeid(a) != typeid(b)) {
            JOYNR_LOG_WARN(logger(),
                           "checkType: Types {} and {} are not equal!",
                           typeid(a).name(),
                           typeid(b).name());
            return false;
        }
        JOYNR_LOG_TRACE(logger(),
                        "checkType: Types {} and {} are equal",
                        typeid(a).name(),
                        typeid(b).name());
        return true;
    }
};
#endif // ILTUTIL_H
