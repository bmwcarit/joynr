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
#ifndef MASTERACCESSCONTROLENTRYSERIALIZER_H
#define MASTERACCESSCONTROLENTRYSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"

#include <sstream>

namespace joynr
{

// Serializes a MasterAccessControlEntry
using MasterAccessControlEntry = infrastructure::DacTypes::MasterAccessControlEntry;
using TrustLevel = infrastructure::DacTypes::TrustLevel;
using Permission = infrastructure::DacTypes::Permission;

template <>
void ClassSerializer<MasterAccessControlEntry>::serialize(const MasterAccessControlEntry& masterAccessControlEntry, std::ostream& stringstream);

// Deserializes a MasterAccessControlEntry
template <>
void ClassDeserializer<MasterAccessControlEntry>::deserialize(MasterAccessControlEntry& masterAccessControlEntry, IObject& object);

// Serialize TrustLevel Enum
template <>
void ClassSerializer<TrustLevel::Enum>::serialize(const TrustLevel::Enum &trustLevel, std::ostream &stringstream);

// Serialize Permission Enum
template <>
void ClassSerializer<Permission::Enum>::serialize(const Permission::Enum &permission, std::ostream &stringstream);

} // end namespace joynr
#endif // MASTERACCESSCONTROLENTRYSERIALIZER_H
