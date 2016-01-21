/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "AceValidator.h"

#include <set>
#include <algorithm>

namespace joynr
{

using namespace infrastructure::DacTypes;

AceValidator::AceValidator(const Optional<MasterAccessControlEntry>& masterAceOptional,
                           const Optional<MasterAccessControlEntry>& mediatorAceOptional,
                           const Optional<OwnerAccessControlEntry>& ownerAceOptional)
        : masterAceOptional(masterAceOptional),
          mediatorAceOptional(mediatorAceOptional),
          ownerAceOptional(ownerAceOptional)
{
}

bool AceValidator::isValid()
{
    return isOwnerValid();
}

bool AceValidator::isOwnerValid()
{
    bool isOwnerValid = true;
    if (mediatorAceOptional) {
        isOwnerValid = isMediatorValid() && validateOwner(mediatorAceOptional.getValue());
    } else if (masterAceOptional) {
        isOwnerValid = validateOwner(masterAceOptional.getValue());
    }

    return isOwnerValid;
}

bool AceValidator::isMediatorValid()
{
    if (!mediatorAceOptional) {
        return true;
    }

    // if mediator ACE is not null and master ACE is null, mediator is valid
    if (!masterAceOptional) {
        return true;
    }

    MasterAccessControlEntry masterAce = masterAceOptional.getValue();
    MasterAccessControlEntry mediatorAce = mediatorAceOptional.getValue();

    bool isMediatorValid = true;

    auto masterPossiblePermissions = vectorToSet(masterAce.getPossibleConsumerPermissions());
    if (!masterPossiblePermissions.count(mediatorAce.getDefaultConsumerPermission())) {
        isMediatorValid = false;
    } else {
        // Convert the lists to sets so that intersections can be easily calculated
        auto mediatorPossiblePermissions =
                vectorToSet(mediatorAce.getPossibleConsumerPermissions());
        if (!setContainsSet(masterPossiblePermissions, mediatorPossiblePermissions)) {
            isMediatorValid = false;
        }
    }

    auto masterPossibleTrustLevels = vectorToSet(masterAce.getPossibleRequiredTrustLevels());
    if (!masterPossibleTrustLevels.count(mediatorAce.getDefaultRequiredTrustLevel())) {
        isMediatorValid = false;
    } else {
        // Convert the lists to sets so that intersections can be easily calculated
        auto mediatorPossibleTrustLevels =
                vectorToSet(mediatorAce.getPossibleRequiredTrustLevels());
        if (!setContainsSet(masterPossibleTrustLevels, mediatorPossibleTrustLevels)) {
            isMediatorValid = false;
        }
    }

    return isMediatorValid;
}

bool AceValidator::validateOwner(MasterAccessControlEntry targetMasterAce)
{
    if (!ownerAceOptional) {
        return true;
    }

    OwnerAccessControlEntry ownerAce = ownerAceOptional.getValue();
    bool isValid = true;
    auto&& possibleConsumerPermissions = targetMasterAce.getPossibleConsumerPermissions();
    auto&& possibleRequiredTrustLevels = targetMasterAce.getPossibleRequiredTrustLevels();
    if (!vectorContains(possibleConsumerPermissions, ownerAce.getConsumerPermission())) {
        isValid = false;
    } else if (!vectorContains(possibleRequiredTrustLevels, ownerAce.getRequiredTrustLevel())) {
        isValid = false;
    }

    return isValid;
}

} // namespace joynr
