/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#ifndef ACCESS_CONTROL_VALIDATOR_H
#define ACCESS_CONTROL_VALIDATOR_H

#include <boost/optional.hpp>

#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterRegistrationControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerRegistrationControlEntry.h"

namespace joynr
{

namespace tags
{
struct Access {
};
struct Registration {
};
} // namespace tags

template <typename Tag>
struct LinkTagWithControlEntries;

template <>
struct LinkTagWithControlEntries<tags::Access> {
    using MasterEntry = infrastructure::DacTypes::MasterAccessControlEntry;
    using MediatorEntry = MasterEntry;
    using OwnerEntry = infrastructure::DacTypes::OwnerAccessControlEntry;
};

template <>
struct LinkTagWithControlEntries<tags::Registration> {
    using MasterEntry = infrastructure::DacTypes::MasterRegistrationControlEntry;
    using MediatorEntry = MasterEntry;
    using OwnerEntry = infrastructure::DacTypes::OwnerRegistrationControlEntry;
};

/**
 * Validates master, mediator and owner (access | registration) control entries.
 */
template <typename Tag>
class Validator
{
private:
    using Link = LinkTagWithControlEntries<Tag>;

public:
    using MasterEntry = typename Link::MasterEntry;
    using MediatorEntry = typename Link::MediatorEntry;
    using OwnerEntry = typename Link::OwnerEntry;

    Validator(const boost::optional<MasterEntry>& masterEntryOptional,
              const boost::optional<MediatorEntry>& mediatorEntryOptional,
              const boost::optional<OwnerEntry>& ownerEntryOptional)
            : _masterEntryOptional(masterEntryOptional),
              _mediatorEntryOptional(mediatorEntryOptional),
              _ownerEntryOptional(ownerEntryOptional)
    {
    }

    ~Validator() = default;

    /**
     * Indicates if the master, mediator and owner entries are compatible with each other
     * @return true if the entries are valid, false otherwise
     */
    bool isValid() const
    {
        return isOwnerValid();
    }

    bool isOwnerValid() const
    {
        bool isOwnerValid = true;
        if (_mediatorEntryOptional) {
            isOwnerValid = isMediatorValid() && validateOwner(*_mediatorEntryOptional);
        } else if (_masterEntryOptional) {
            isOwnerValid = validateOwner(*_masterEntryOptional);
        }

        return isOwnerValid;
    }

    bool isMediatorValid() const
    {
        // if mediator entry is missing, always return true
        if (!_mediatorEntryOptional) {
            return true;
        }

        // if master entry is not set, mediator is valid
        if (!_masterEntryOptional) {
            return true;
        }

        bool isMediatorValid = true;

        auto masterPossiblePermissions =
                util::vectorToSet(getPossiblePermissions(*_masterEntryOptional));
        if (masterPossiblePermissions.count(getDefaultPermission(*_mediatorEntryOptional)) == 0) {
            isMediatorValid = false;
        } else {
            // Convert the lists to sets so that intersections can be easily calculated
            auto mediatorPossiblePermissions =
                    util::vectorToSet(getPossiblePermissions(*_mediatorEntryOptional));
            if (!util::setContainsSet(masterPossiblePermissions, mediatorPossiblePermissions)) {
                isMediatorValid = false;
            }
        }

        auto masterPossibleTrustLevels =
                util::vectorToSet(_masterEntryOptional->getPossibleRequiredTrustLevels());
        if (masterPossibleTrustLevels.count(
                    _mediatorEntryOptional->getDefaultRequiredTrustLevel()) == 0) {
            isMediatorValid = false;
        } else {
            // Convert the lists to sets so that intersections can be easily calculated
            auto mediatorPossibleTrustLevels =
                    util::vectorToSet(_mediatorEntryOptional->getPossibleRequiredTrustLevels());
            if (!util::setContainsSet(masterPossibleTrustLevels, mediatorPossibleTrustLevels)) {
                isMediatorValid = false;
            }
        }

        return isMediatorValid;
    }

private:
    boost::optional<MasterEntry> _masterEntryOptional;
    boost::optional<MediatorEntry> _mediatorEntryOptional;
    boost::optional<OwnerEntry> _ownerEntryOptional;

    bool validateOwner(const MasterEntry& targetMasterEntry) const
    {
        // if owner entry is missing, always return true
        if (!_ownerEntryOptional) {
            return true;
        }

        bool isValid = true;
        auto&& possibleRequiredTrustLevels = targetMasterEntry.getPossibleRequiredTrustLevels();
        if (!ownerPermissionInPossiblePermissions(targetMasterEntry)) {
            isValid = false;
        } else if (!util::vectorContains(possibleRequiredTrustLevels,
                                         _ownerEntryOptional->getRequiredTrustLevel())) {
            isValid = false;
        }

        return isValid;
    }

    bool ownerPermissionInPossiblePermissions(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& targetMasterEntry) const
    {
        const auto& possiblePermissions = targetMasterEntry.getPossibleProviderPermissions();
        return util::vectorContains(
                possiblePermissions, _ownerEntryOptional->getProviderPermission());
    }

    bool ownerPermissionInPossiblePermissions(
            const infrastructure::DacTypes::MasterAccessControlEntry& targetMasterEntry) const
    {
        const auto& possiblePermissions = targetMasterEntry.getPossibleConsumerPermissions();
        return util::vectorContains(
                possiblePermissions, _ownerEntryOptional->getConsumerPermission());
    }

    auto getPossiblePermissions(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& entry) const
    {
        return entry.getPossibleProviderPermissions();
    }

    auto getPossiblePermissions(
            const infrastructure::DacTypes::MasterAccessControlEntry& entry) const
    {
        return entry.getPossibleConsumerPermissions();
    }

    auto getDefaultPermission(
            const infrastructure::DacTypes::MasterRegistrationControlEntry& entry) const
    {
        return entry.getDefaultProviderPermission();
    }

    auto getDefaultPermission(const infrastructure::DacTypes::MasterAccessControlEntry& entry) const
    {
        return entry.getDefaultConsumerPermission();
    }
};

using AceValidator = Validator<tags::Access>;
using RceValidator = Validator<tags::Registration>;

} // namespace joynr
#endif // ACCESS_CONTROL_VALIDATOR_H
