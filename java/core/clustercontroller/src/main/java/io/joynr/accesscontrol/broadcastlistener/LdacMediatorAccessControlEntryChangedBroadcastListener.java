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
import joynr.infrastructure.GlobalDomainAccessControllerBroadcastInterface.MediatorAccessControlEntryChangedBroadcastAdapter;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class LdacMediatorAccessControlEntryChangedBroadcastListener extends
        MediatorAccessControlEntryChangedBroadcastAdapter {
    private static final Logger LOG = LoggerFactory.getLogger(LdacMediatorAccessControlEntryChangedBroadcastListener.class);

    private DomainAccessControlStore localDomainAccessStore;

    public LdacMediatorAccessControlEntryChangedBroadcastListener(DomainAccessControlStore domainAccessControlStore) {
        this.localDomainAccessStore = domainAccessControlStore;
    }

    @Override
    public void onReceive(ChangeType typeOfChange, MasterAccessControlEntry newMediatorAce) {
        if (!typeOfChange.equals(ChangeType.REMOVE)) {
            localDomainAccessStore.updateMediatorAccessControlEntry(newMediatorAce);
            LOG.debug("Updated mediator ACE: {}", newMediatorAce.toString());
        } else {
            // removes are notified using entries where all fields except the key fields are null
            localDomainAccessStore.removeMediatorAccessControlEntry(newMediatorAce.getUid(),
                                                                    newMediatorAce.getDomain(),
                                                                    newMediatorAce.getInterfaceName(),
                                                                    newMediatorAce.getOperation());
            LOG.debug("Removed mediator ACE: {}", newMediatorAce.toString());
        }
    }

    @Override
    public void onError(SubscriptionException error) {
        LOG.error("Subscription to mediatorAce failed! SubscriptionId: {}, error: {}",
                  error.getSubscriptionId(),
                  error.getMessage());
    }
}
