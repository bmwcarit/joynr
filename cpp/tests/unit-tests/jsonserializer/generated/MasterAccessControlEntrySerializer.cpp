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
#include "MasterAccessControlEntrySerializer.h"
#include "joynr/ArraySerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

#include <string>
#include <utility>
#include <algorithm>

namespace joynr
{

using namespace joynr::infrastructure::DacTypes;

// Register the MasterAccessControlEntry type id (_typeName value) and serializer/deserializer
static const bool isMasterAccessControlEntrySerializerRegistered =
        SerializerRegistry::registerType<MasterAccessControlEntry>("joynr.infrastructure.DacTypes.MasterAccessControlEntry");

// deserialize to TrustLevel
TrustLevel::Enum convertToTrustLevelEnum(IValue& value)
{
    std::string text = value;

    // following could be done much simpler if TrustLevel type could cast from std::string
    if (text == "HIGH") {
        return TrustLevel::HIGH;
    } else if (text == "LOW") {
        return TrustLevel::LOW;
    } else if (text == "MID") {
        return TrustLevel::MID;
    } else if (text == "NONE") {
        return TrustLevel::NONE;
    } else {
        throw std::invalid_argument("Unknown enum value");
    }
}

// deserialize to Permission Enum
Permission::Enum convertToPermissionEnum(IValue& value)
{
    std::string text = value;

    // following could be done much simpler if Permission type could cast from std::string
    if (text == "YES") {
        return Permission::YES;
    } else if (text == "NO") {
        return Permission::NO;
    } else if (text == "ASK") {
        return Permission::ASK;
    } else {
        throw std::invalid_argument("Unknown enum value");
    }
}

template <>
void ClassDeserializer<MasterAccessControlEntry>::deserialize(MasterAccessControlEntry &masterAccessControlEntry, IObject &object)
{
    while (object.hasNextField()) {
        IField& field = object.nextField();
        if (field.name() == "uid") {
            masterAccessControlEntry.setUid(field.value());
        } else if (field.name() == "domain") {
            masterAccessControlEntry.setDomain(field.value());
        } else if (field.name() == "interfaceName") {
            masterAccessControlEntry.setInterfaceName(field.value());
        } else if (field.name() == "defaultRequiredTrustLevel") {
            masterAccessControlEntry.setDefaultRequiredTrustLevel(convertToTrustLevelEnum(field.value()));
        } else if (field.name() == "possibleRequiredTrustLevels") {
            IArray& array = field.value();
            auto&& converted = convertArray<TrustLevel::Enum>(array, convertToTrustLevelEnum);
            masterAccessControlEntry.setPossibleRequiredTrustLevels(std::forward<std::vector<TrustLevel::Enum>>(converted));
        } else if (field.name() == "defaultRequiredControlEntryChangeTrustLevel") {
            masterAccessControlEntry.setDefaultRequiredControlEntryChangeTrustLevel(convertToTrustLevelEnum(field.value()));
        } else if (field.name() == "possibleRequiredControlEntryChangeTrustLevels") {
            IArray& array = field.value();
            auto&& converted = convertArray<TrustLevel::Enum>(array, convertToTrustLevelEnum);
            masterAccessControlEntry.setPossibleRequiredControlEntryChangeTrustLevels(std::forward<std::vector<TrustLevel::Enum>>(converted));
        } else if (field.name() == "operation") {
            masterAccessControlEntry.setOperation(field.value());
        } else if (field.name() == "defaultConsumerPermission") {
            masterAccessControlEntry.setDefaultConsumerPermission(convertToPermissionEnum(field.value()));
        } else if (field.name() == "possibleConsumerPermissions") {
            IArray& array = field.value();
            auto&& converted = convertArray<Permission::Enum>(array, convertToPermissionEnum);
            masterAccessControlEntry.setPossibleConsumerPermissions(std::forward<std::vector<Permission::Enum>>(converted));
        }
    }
}

template <>
void ClassSerializer<MasterAccessControlEntry>::serialize(const MasterAccessControlEntry& masterAccessControlEntry, std::ostream& stream)
{
    stream << "{";
    stream << "\"_typeName\": \"" << JoynrTypeId<MasterAccessControlEntry>::getTypeName() << "\",";
    stream << "\"uid\": \"" << masterAccessControlEntry.getUid() << "\",";
    stream << "\"domain\": \"" << masterAccessControlEntry.getDomain() << "\",";
    stream << "\"interfaceName\": \"" << masterAccessControlEntry.getInterfaceName() << "\",";
    stream << "\"defaultRequiredTrustLevel\": \"" << TrustLevel::getLiteral(masterAccessControlEntry.getDefaultRequiredTrustLevel()) << "\",";
    stream << "\"possibleRequiredTrustLevels\": ";
    ArraySerializer::serialize<TrustLevel::Enum>(masterAccessControlEntry.getPossibleRequiredTrustLevels(), stream);
    stream << ",";
    stream << "\"defaultRequiredControlEntryChangeTrustLevel\": \"" << TrustLevel::getLiteral(masterAccessControlEntry.getDefaultRequiredControlEntryChangeTrustLevel()) << "\",";
    stream << "\"possibleRequiredControlEntryChangeTrustLevels\": ";
    ArraySerializer::serialize<TrustLevel::Enum>(masterAccessControlEntry.getPossibleRequiredControlEntryChangeTrustLevels(), stream);
    stream << ",";
    stream << "\"operation\": \"" << masterAccessControlEntry.getOperation() << "\",";
    stream << "\"defaultConsumerPermission\": \"" << Permission::getLiteral(masterAccessControlEntry.getDefaultConsumerPermission()) << "\",";
    stream << "\"possibleConsumerPermissions\": ";
    ArraySerializer::serialize<Permission::Enum>(masterAccessControlEntry.getPossibleConsumerPermissions(), stream);
    stream << "}";

}

template <>
void ClassSerializer<TrustLevel::Enum>::serialize(const TrustLevel::Enum &trustLevel, std::ostream &stream)
{
    stream << "\""<< TrustLevel::getLiteral(trustLevel) << "\"";
}

template <>
void ClassSerializer<Permission::Enum>::serialize(const Permission::Enum &permission, std::ostream &stream)
{
    stream << "\"" << Permission::getLiteral(permission) << "\"";
}

} // end namespace joynr
