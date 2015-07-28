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

#include <QSet>

namespace joynr
{

using namespace infrastructure;

AceValidator::AceValidator(const Optional<QtMasterAccessControlEntry>& masterAceOptional,
                           const Optional<QtMasterAccessControlEntry>& mediatorAceOptional,
                           const Optional<QtOwnerAccessControlEntry>& ownerAceOptional)
        : masterAceOptional(masterAceOptional),
          mediatorAceOptional(mediatorAceOptional),
          ownerAceOptional(ownerAceOptional)
{
}

AceValidator::~AceValidator()
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

    QtMasterAccessControlEntry masterAce = masterAceOptional.getValue();
    QtMasterAccessControlEntry mediatorAce = mediatorAceOptional.getValue();
    bool isMediatorValid = true;
    if (!masterAce.getPossibleConsumerPermissions().contains(
                mediatorAce.getDefaultConsumerPermission())) {
        isMediatorValid = false;
    } else {
        // Convert the lists to sets so that intersections can be easily calculated
        QSet<QtPermission::Enum> masterPossiblePermissions =
                masterAce.getPossibleConsumerPermissions().toSet();
        QSet<QtPermission::Enum> mediatorPossiblePermissions =
                mediatorAce.getPossibleConsumerPermissions().toSet();
        if (!masterPossiblePermissions.contains(mediatorPossiblePermissions)) {
            isMediatorValid = false;
        }
    }
    if (!masterAce.getPossibleRequiredTrustLevels().contains(
                mediatorAce.getDefaultRequiredTrustLevel())) {
        isMediatorValid = false;
    } else {
        // Convert the lists to sets so that intersections can be easily calculated
        QSet<QtTrustLevel::Enum> masterPossibleTrustLevels =
                masterAce.getPossibleRequiredTrustLevels().toSet();
        QSet<QtTrustLevel::Enum> mediatorPossibleTrustLevels =
                mediatorAce.getPossibleRequiredTrustLevels().toSet();
        if (!masterPossibleTrustLevels.contains(mediatorPossibleTrustLevels)) {
            isMediatorValid = false;
        }
    }

    return isMediatorValid;
}

bool AceValidator::validateOwner(QtMasterAccessControlEntry targetMasterAce)
{
    if (!ownerAceOptional) {
        return true;
    }

    QtOwnerAccessControlEntry ownerAce = ownerAceOptional.getValue();
    bool isValid = true;
    if (!targetMasterAce.getPossibleConsumerPermissions().contains(
                ownerAce.getConsumerPermission())) {
        isValid = false;
    } else if (!targetMasterAce.getPossibleRequiredTrustLevels().contains(
                       ownerAce.getRequiredTrustLevel())) {
        isValid = false;
    }

    return isValid;
}

} // namespace joynr
