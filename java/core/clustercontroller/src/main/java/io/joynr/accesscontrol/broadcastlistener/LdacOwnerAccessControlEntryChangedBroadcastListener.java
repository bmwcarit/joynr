package io.joynr.accesscontrol.broadcastlistener;

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

import io.joynr.accesscontrol.DomainAccessControlStore;
import io.joynr.exceptions.SubscriptionException;
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.OwnerAccessControlEntryChangedBroadcastAdapter;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class LdacOwnerAccessControlEntryChangedBroadcastListener extends OwnerAccessControlEntryChangedBroadcastAdapter {
    private static final Logger LOG = LoggerFactory.getLogger(LdacOwnerAccessControlEntryChangedBroadcastListener.class);

    private DomainAccessControlStore localDomainAccessStore;

    public LdacOwnerAccessControlEntryChangedBroadcastListener(DomainAccessControlStore domainAccessControlStore) {
        this.localDomainAccessStore = domainAccessControlStore;
    }

    public void onReceive(ChangeType typeOfChange, OwnerAccessControlEntry newOwnerAce) {
        if (!typeOfChange.equals(ChangeType.REMOVE)) {
            localDomainAccessStore.updateOwnerAccessControlEntry(newOwnerAce);
            LOG.debug("Updated owner ACE: {}", newOwnerAce.toString());
        } else {
            // removes are notified using entries where all fields except the key fields are null
            localDomainAccessStore.removeOwnerAccessControlEntry(newOwnerAce.getUid(),
                                                                 newOwnerAce.getDomain(),
                                                                 newOwnerAce.getInterfaceName(),
                                                                 newOwnerAce.getOperation());
            LOG.debug("Removed owner ACE: {}", newOwnerAce.toString());
        }
    }

    public void onError(SubscriptionException error) {
        LOG.error("Subscription to ownerAce failed! SubscriptionId: {}, error: {}",
                  error.getSubscriptionId(),
                  error.getMessage());
    }
}
