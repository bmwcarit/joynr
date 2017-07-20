package io.joynr.accesscontrol;

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

import javax.annotation.Nullable;

import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

/**
 * Algorithm that aggregates permission from all relevant control entries.
 */
public class AccessControlAlgorithm {

    /**
     * Get the consumer permission for given combination of control entries and with the given trust level.
     *
     * @param master     The master access control entry
     * @param mediator   The mediator access control entry
     * @param owner      The owner access control entry
     * @param trustLevel The trust level of the user sending the message
     * @return consumer permission
     */
    public Permission getConsumerPermission(@Nullable MasterAccessControlEntry master,
                                            @Nullable MasterAccessControlEntry mediator,
                                            @Nullable OwnerAccessControlEntry owner,
                                            TrustLevel trustLevel) {

        return getPermission(PermissionType.CONSUMER, master, mediator, owner, trustLevel);
    }

    /**
     * Get the provider permission for given combination of control entries and with the given trust level.
     *
     * @param master     The master access control entry
     * @param mediator   The mediator access control entry
     * @param owner      The owner access control entry
     * @param trustLevel The trust level of the user sending the message
     * @return provider permission
     */
    public Permission getProviderPermission(@Nullable MasterAccessControlEntry master,
                                            @Nullable MasterAccessControlEntry mediator,
                                            @Nullable OwnerAccessControlEntry owner,
                                            TrustLevel trustLevel) {
        assert (false) : "Provider permission algorithm is not yet implemented!";
        return getPermission(PermissionType.PROVIDER, master, mediator, owner, trustLevel);
    }

    private Permission getPermission(PermissionType type,
                                     @Nullable MasterAccessControlEntry masterAce,
                                     @Nullable MasterAccessControlEntry mediatorAce,
                                     @Nullable OwnerAccessControlEntry ownerAce,
                                     TrustLevel trustLevel) {
        AceValidator aceValidator = new AceValidator(masterAce, mediatorAce, ownerAce);
        if (!aceValidator.isValid()) {
            return Permission.NO;
        }

        Permission permission = Permission.NO;
        if (ownerAce != null) {
            if (TrustLevelComparator.compare(trustLevel, ownerAce.getRequiredTrustLevel()) >= 0) {
                if (type == PermissionType.CONSUMER) {
                    permission = ownerAce.getConsumerPermission();
                }
            }
        } else if (mediatorAce != null) {
            if (TrustLevelComparator.compare(trustLevel, mediatorAce.getDefaultRequiredTrustLevel()) >= 0) {
                if (type == PermissionType.CONSUMER) {
                    permission = mediatorAce.getDefaultConsumerPermission();
                }
            }
        } else if (masterAce != null) {
            if (TrustLevelComparator.compare(trustLevel, masterAce.getDefaultRequiredTrustLevel()) >= 0) {
                if (type == PermissionType.CONSUMER) {
                    permission = masterAce.getDefaultConsumerPermission();
                }
            }
        }

        return permission;
    }

    private enum PermissionType {
        PROVIDER, CONSUMER
    }
}
