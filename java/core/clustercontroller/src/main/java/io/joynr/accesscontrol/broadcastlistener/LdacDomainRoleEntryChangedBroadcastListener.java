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
package io.joynr.accesscontrol.broadcastlistener;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.accesscontrol.DomainAccessControlStore;
import io.joynr.exceptions.SubscriptionException;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.GlobalDomainRoleControllerBroadcastInterface.DomainRoleEntryChangedBroadcastAdapter;

public class LdacDomainRoleEntryChangedBroadcastListener extends DomainRoleEntryChangedBroadcastAdapter {
    private static final Logger logger = LoggerFactory.getLogger(LdacDomainRoleEntryChangedBroadcastListener.class);
    private final DomainAccessControlStore localDomainAccessStore;

    public LdacDomainRoleEntryChangedBroadcastListener(DomainAccessControlStore domainAccessControlStore) {
        this.localDomainAccessStore = domainAccessControlStore;
    }

    @Override
    public void onReceive(ChangeType typeOfChange, DomainRoleEntry newDomainRoleEntry) {
        if (!typeOfChange.equals(ChangeType.REMOVE)) {
            localDomainAccessStore.updateDomainRole(newDomainRoleEntry);
            logger.debug("Updated DRE: {}", newDomainRoleEntry.toString());
        } else {
            // removes are notified using entries where all fields except the key fields are null
            localDomainAccessStore.removeDomainRole(newDomainRoleEntry.getUid(), newDomainRoleEntry.getRole());
            logger.debug("Removed DRE: {}", newDomainRoleEntry.toString());
        }
    }

    @Override
    public void onError(SubscriptionException error) {
        logger.error("Subscription to DRE failed! SubscriptionId: {}, error: {}",
                     error.getSubscriptionId(),
                     error.getMessage());
    }
}
