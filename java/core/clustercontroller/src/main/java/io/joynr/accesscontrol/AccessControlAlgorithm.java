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
package io.joynr.accesscontrol;

import java.util.Optional;

import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

/**
 * Algorithm that aggregates permission from all relevant control entries.
 */
public class AccessControlAlgorithm {

    /**
     * Get the consumer permission for given combination of control entries and with the given trust level.
     *
     * @param masterAce     The master access control entry
     * @param mediatorAce   The mediator access control entry
     * @param ownerAce      The owner access control entry
     * @param trustLevel The trust level of the user sending the message
     * @return consumer permission
     */
    public Permission getConsumerPermission(Optional<MasterAccessControlEntry> masterAce,
                                            Optional<MasterAccessControlEntry> mediatorAce,
                                            Optional<OwnerAccessControlEntry> ownerAce,
                                            TrustLevel trustLevel) {
        AceValidator aceValidator = new AceValidator(masterAce.isPresent() ? masterAce.get() : null,
                                                     mediatorAce.isPresent() ? mediatorAce.get() : null,
                                                     ownerAce.isPresent() ? ownerAce.get() : null);
        if (!aceValidator.isValid()) {
            return Permission.NO;
        }

        Permission permission = Permission.NO;
        if (ownerAce.isPresent()) {
            if (TrustLevelComparator.compare(trustLevel, ownerAce.get().getRequiredTrustLevel()) >= 0) {
                permission = ownerAce.get().getConsumerPermission();
            }
        } else if (mediatorAce.isPresent()) {
            if (TrustLevelComparator.compare(trustLevel, mediatorAce.get().getDefaultRequiredTrustLevel()) >= 0) {
                permission = mediatorAce.get().getDefaultConsumerPermission();
            }
        } else if (masterAce.isPresent()) {
            if (TrustLevelComparator.compare(trustLevel, masterAce.get().getDefaultRequiredTrustLevel()) >= 0) {
                permission = masterAce.get().getDefaultConsumerPermission();
            }
        }

        return permission;
    }

    /**
     * Get the provider permission for given combination of control entries and with the given trust level.
     *
     * @param masterRce     The master access control entry
     * @param mediatorRce   The mediator access control entry
     * @param ownerRce      The owner access control entry
     * @param trustLevel The trust level of the user sending the message
     * @return provider permission
     */
    public Permission getProviderPermission(Optional<MasterRegistrationControlEntry> masterRce,
                                            Optional<MasterRegistrationControlEntry> mediatorRce,
                                            Optional<OwnerRegistrationControlEntry> ownerRce,
                                            TrustLevel trustLevel) {
        RceValidator rceValidator = new RceValidator(masterRce.isPresent() ? masterRce.get() : null,
                                                     mediatorRce.isPresent() ? mediatorRce.get() : null,
                                                     ownerRce.isPresent() ? ownerRce.get() : null);
        if (!rceValidator.isValid()) {
            return Permission.NO;
        }

        Permission permission = Permission.NO;
        if (ownerRce.isPresent()) {
            if (TrustLevelComparator.compare(trustLevel, ownerRce.get().getRequiredTrustLevel()) >= 0) {
                permission = ownerRce.get().getProviderPermission();
            }
        } else if (mediatorRce.isPresent()) {
            if (TrustLevelComparator.compare(trustLevel, mediatorRce.get().getDefaultRequiredTrustLevel()) >= 0) {
                permission = mediatorRce.get().getDefaultProviderPermission();
            }
        } else if (masterRce.isPresent()) {
            if (TrustLevelComparator.compare(trustLevel, masterRce.get().getDefaultRequiredTrustLevel()) >= 0) {
                permission = masterRce.get().getDefaultProviderPermission();
            }
        }

        return permission;
    }
}
