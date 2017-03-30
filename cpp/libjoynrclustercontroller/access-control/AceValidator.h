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

#ifndef ACEVALIDATOR_H
#define ACEVALIDATOR_H

#include <boost/optional.hpp>

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"

namespace joynr
{

/**
 * Validates master, mediator and owner access control entries.
 */
class AceValidator
{
public:
    AceValidator(const boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>&
                         masterAceOptional,
                 const boost::optional<infrastructure::DacTypes::MasterAccessControlEntry>&
                         mediatorAceOptional,
                 const boost::optional<infrastructure::DacTypes::OwnerAccessControlEntry>&
                         ownerAceOptional);

    ~AceValidator() = default;

    /**
     * Indicates if the master, mediator and owner entries are compatible with each other
     * @return true if the entries are valid, false otherwise
     */
    bool isValid();
    bool isOwnerValid();
    bool isMediatorValid();

private:
    boost::optional<infrastructure::DacTypes::MasterAccessControlEntry> masterAceOptional;
    boost::optional<infrastructure::DacTypes::MasterAccessControlEntry> mediatorAceOptional;
    boost::optional<infrastructure::DacTypes::OwnerAccessControlEntry> ownerAceOptional;

    bool validateOwner(const infrastructure::DacTypes::MasterAccessControlEntry& targetMasterAce);
};

} // namespace joynr
#endif // ACEVALIDATOR_H
