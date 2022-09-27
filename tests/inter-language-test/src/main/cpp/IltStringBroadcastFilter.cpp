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
#include "IltStringBroadcastFilter.h"

#include <string>

#include "IltUtil.h"
#include "joynr/serializer/Serializer.h"

IltStringBroadcastFilter::IltStringBroadcastFilter()
{
}

bool IltStringBroadcastFilter::filter(
        const std::string& stringOut,
        const std::vector<std::string>& stringArrayOut,
        const joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut,
        const joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                structWithStringArrayOut,
        const std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                structWithStringArrayArrayOut,
        const interlanguagetest::TestInterfaceBroadcastWithFilteringBroadcastFilterParameters&
                filterParameters)
{
    JOYNR_LOG_WARN(logger(), "****************************************");
    JOYNR_LOG_WARN(logger(), "* IltStringBroadcastFilter::filter called");
    JOYNR_LOG_WARN(logger(), "****************************************");

    // check output parameter contents
    bool checkResult = true;

    if (!IltUtil::checkStringArray(stringArrayOut)) {
        JOYNR_LOG_WARN(
                logger(), "IltStringBroadcastFilter::filter: stringArrayOut has invalid value");
        JOYNR_LOG_WARN(logger(), "array size is {}", stringArrayOut.size());
        checkResult = false;
    }
    if (enumerationOut != joynr::interlanguagetest::namedTypeCollection2::
                                  ExtendedTypeCollectionEnumerationInTypeCollection::
                                          ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
        JOYNR_LOG_WARN(
                logger(), "IltStringBroadcastFilter::filter: enumerationOut has invalid value");
        checkResult = false;
    }
    if (!IltUtil::checkStructWithStringArray(structWithStringArrayOut)) {
        JOYNR_LOG_WARN(
                logger(),
                "IltStringBroadcastFilter::filter: structWithStringArrayOut has invalid value");
        checkResult = false;
    }

    if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
        JOYNR_LOG_WARN(logger(),
                       "IltStringBroadcastFilter::filter: structWithStringArrayArrayOut "
                       "has invalid value");
        checkResult = false;
    }

    if (checkResult == false) {
        JOYNR_LOG_WARN(logger(), "IltStringBroadcastFilter::filter: check of out args failed");
        return false;
    }

    // check filter parameter contents

    // variables to store deserialized values
    std::string stringOfInterest = filterParameters.getStringOfInterest();
    JOYNR_LOG_WARN(
            logger(), "IltStringBroadcastFilter::filter: stringOfInterest = {}", stringOfInterest);
    JOYNR_LOG_WARN(logger(), "IltStringBroadcastFilter::filter: stringOut = {}", stringOut);

    std::vector<std::string> stringArrayOfInterest;
    joynr::serializer::deserializeFromJson(
            stringArrayOfInterest, filterParameters.getStringArrayOfInterest());

    std::string enumerationOfInterestJson = filterParameters.getEnumerationOfInterest();
    std::string enumerationOfInterestLiteral =
            enumerationOfInterestJson.substr(1, enumerationOfInterestJson.size() - 2);
    joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::Enum enumerationOfInterest =
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedTypeCollectionEnumerationInTypeCollection::getEnum(
                                    enumerationOfInterestLiteral);

    try {
        using joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray;

        StructWithStringArray structWithStringArrayOfInterest;
        joynr::serializer::deserializeFromJson(
                structWithStringArrayOfInterest,
                filterParameters.getStructWithStringArrayOfInterest());

        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                structWithStringArrayArrayOfInterest;
        joynr::serializer::deserializeFromJson(
                structWithStringArrayArrayOfInterest,
                filterParameters.getStructWithStringArrayArrayOfInterest());

        if (!IltUtil::checkStringArray(stringArrayOfInterest)) {
            return false;
        }
        if (enumerationOfInterest != joynr::interlanguagetest::namedTypeCollection2::
                                             ExtendedTypeCollectionEnumerationInTypeCollection::
                                                     ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            return false;
        }
        if (!IltUtil::checkStructWithStringArray(structWithStringArrayOfInterest)) {
            return false;
        }
        if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOfInterest)) {
            return false;
        }

    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger(),
                        "could not deserialize StructWithStringArray from {} - error: {}",
                        filterParameters.getStructWithStringArrayOfInterest(),
                        e.what());
        return false;
    }

    // decision for publication is made base on stringOfInterest
    if (stringOut == stringOfInterest) {
        JOYNR_LOG_WARN(logger(), "IltStringBroadcastFilter::filter: returns true");
        return true;
    } else {
        JOYNR_LOG_WARN(logger(), "IltStringBroadcastFilter::filter: returns false");
        return false;
    }
}
