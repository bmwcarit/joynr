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

#include "AceValidator.h"

#include <algorithm>
#include <set>

namespace joynr
{

using namespace infrastructure::DacTypes;

AceValidator::AceValidator(const boost::optional<MasterAccessControlEntry>& masterAceOptional,
                           const boost::optional<MasterAccessControlEntry>& mediatorAceOptional,
                           const boost::optional<OwnerAccessControlEntry>& ownerAceOptional)
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
        isOwnerValid = isMediatorValid() && validateOwner(*mediatorAceOptional);
    } else if (masterAceOptional) {
        isOwnerValid = validateOwner(*masterAceOptional);
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

    bool isMediatorValid = true;

    auto masterPossiblePermissions =
            util::vectorToSet(masterAceOptional->getPossibleConsumerPermissions());
    if (!masterPossiblePermissions.count(mediatorAceOptional->getDefaultConsumerPermission())) {
        isMediatorValid = false;
    } else {
        // Convert the lists to sets so that intersections can be easily calculated
        auto mediatorPossiblePermissions =
                util::vectorToSet(mediatorAceOptional->getPossibleConsumerPermissions());
        if (!util::setContainsSet(masterPossiblePermissions, mediatorPossiblePermissions)) {
            isMediatorValid = false;
        }
    }

    auto masterPossibleTrustLevels =
            util::vectorToSet(masterAceOptional->getPossibleRequiredTrustLevels());
    if (!masterPossibleTrustLevels.count(mediatorAceOptional->getDefaultRequiredTrustLevel())) {
        isMediatorValid = false;
    } else {
        // Convert the lists to sets so that intersections can be easily calculated
        auto mediatorPossibleTrustLevels =
                util::vectorToSet(mediatorAceOptional->getPossibleRequiredTrustLevels());
        if (!util::setContainsSet(masterPossibleTrustLevels, mediatorPossibleTrustLevels)) {
            isMediatorValid = false;
        }
    }

    return isMediatorValid;
}

bool AceValidator::validateOwner(const MasterAccessControlEntry& targetMasterAce)
{
    if (!ownerAceOptional) {
        return true;
    }

    bool isValid = true;
    auto&& possibleConsumerPermissions = targetMasterAce.getPossibleConsumerPermissions();
    auto&& possibleRequiredTrustLevels = targetMasterAce.getPossibleRequiredTrustLevels();
    if (!util::vectorContains(
                possibleConsumerPermissions, ownerAceOptional->getConsumerPermission())) {
        isValid = false;
    } else if (!util::vectorContains(
                       possibleRequiredTrustLevels, ownerAceOptional->getRequiredTrustLevel())) {
        isValid = false;
    }

    return isValid;
}

} // namespace joynr
