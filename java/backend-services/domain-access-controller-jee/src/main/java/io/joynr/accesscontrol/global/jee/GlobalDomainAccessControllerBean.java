package io.joynr.accesscontrol.global.jee;

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

import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MASTER;
import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MEDIATOR;

import javax.ejb.Stateless;
import javax.ejb.Schedule;
import javax.inject.Inject;
import javax.transaction.Transactional;

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import static io.joynr.dispatching.subscription.MulticastIdUtil.sanitizeForPartition;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.GlobalDomainAccessControllerSubscriptionPublisher;
import joynr.infrastructure.GlobalDomainAccessControllerSync;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Stateless
@ServiceProvider(serviceInterface = GlobalDomainAccessControllerSync.class)
@Transactional
public class GlobalDomainAccessControllerBean implements GlobalDomainAccessControllerSync {
    private static final Logger logger = LoggerFactory.getLogger(GlobalDomainAccessControllerBean.class);

    private MasterAccessControlEntryManager masterAccessControlEntryManager;

    private OwnerAccessControlEntryManager ownerAccessControlEntryManager;

    private MasterRegistrationControlEntryManager masterRegistrationControlEntryManager;

    private OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager;

    private GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher;

    private GlobalDomainAccessControllerQueue globalDomainAccessControllerQueue;

    // Only required for testing
    protected GlobalDomainAccessControllerBean() {
    }

    @Inject
    public GlobalDomainAccessControllerBean(@SubscriptionPublisher GlobalDomainAccessControllerSubscriptionPublisher globalDomainAccessControllerSubscriptionPublisher,
                                            MasterAccessControlEntryManager masterAccessControlEntryManager,
                                            OwnerAccessControlEntryManager ownerAccessControlEntryManager,
                                            MasterRegistrationControlEntryManager masterRegistrationControlEntryManager,
                                            OwnerRegistrationControlEntryManager ownerRegistrationControlEntryManager,
                                            GlobalDomainAccessControllerQueue globalDomainAccessControllerQueue) {
        this.globalDomainAccessControllerSubscriptionPublisher = globalDomainAccessControllerSubscriptionPublisher;
        this.masterAccessControlEntryManager = masterAccessControlEntryManager;
        this.ownerAccessControlEntryManager = ownerAccessControlEntryManager;
        this.masterRegistrationControlEntryManager = masterRegistrationControlEntryManager;
        this.ownerRegistrationControlEntryManager = ownerRegistrationControlEntryManager;
        this.globalDomainAccessControllerQueue = globalDomainAccessControllerQueue;
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserId(uid, MASTER);
    }

    @Override
    public MasterAccessControlEntry[] getMasterAccessControlEntries(String domain, String interfaceName) {
        return masterAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName, MASTER);
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String uid) {
        return masterAccessControlEntryManager.findByUserId(uid, MEDIATOR);
    }

    @Override
    public MasterAccessControlEntry[] getMediatorAccessControlEntries(String domain, String interfaceName) {
        return masterAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName, MEDIATOR);
    }

    @Override
    public OwnerAccessControlEntry[] getOwnerAccessControlEntries(String uid) {
        return ownerAccessControlEntryManager.findByUserId(uid);
    }

    @Override
    public OwnerAccessControlEntry[] getOwnerAccessControlEntries(String domain, String interfaceName) {
        return ownerAccessControlEntryManager.findByDomainAndInterfaceName(domain, interfaceName);
    }

    @Override
    public MasterRegistrationControlEntry[] getMasterRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndType(uid, ControlEntryType.MASTER);
    }

    @Override
    public MasterRegistrationControlEntry[] getMediatorRegistrationControlEntries(String uid) {
        return masterRegistrationControlEntryManager.findByUserIdAndType(uid, ControlEntryType.MEDIATOR);
    }

    @Override
    public OwnerRegistrationControlEntry[] getOwnerRegistrationControlEntries(String uid) {
        return ownerRegistrationControlEntryManager.findByUserId(uid);
    }

    @Schedule(second = "*/1", minute = "*", hour = "*", persistent = false)
    public void handleQueuedBroadcastPublications() {
        while (!globalDomainAccessControllerQueue.isEmpty()) {
            try {
                GlobalDomainAccessControllerQueueJob job = globalDomainAccessControllerQueue.take();

                if (job.getMasterAccessControlEntry() != null) {
                    globalDomainAccessControllerSubscriptionPublisher.fireMasterAccessControlEntryChanged(
                            job.getChangeType(),
                            job.getMasterAccessControlEntry(),
                            sanitizeForPartition(job.getMasterAccessControlEntry().getUid()),
                            sanitizeForPartition(job.getMasterAccessControlEntry().getDomain()),
                            sanitizeForPartition(job.getMasterAccessControlEntry().getInterfaceName()));
                    // TODO: add
                    // sanitizeForPartition(job.getMasterAccessControlEntry().getOperation())
                } else if (job.getMediatorAccessControlEntry() != null) {
                    globalDomainAccessControllerSubscriptionPublisher.fireMediatorAccessControlEntryChanged(
                            job.getChangeType(),
                            job.getMediatorAccessControlEntry(),
                            sanitizeForPartition(job.getMediatorAccessControlEntry().getUid()),
                            sanitizeForPartition(job.getMediatorAccessControlEntry().getDomain()),
                            sanitizeForPartition(job.getMediatorAccessControlEntry().getInterfaceName()));
                    // TODO: add
                    // sanitizeForPartition(job.getMasterAccessControlEntry().getOperation())
                } else if (job.getOwnerAccessControlEntry() != null) {
                    globalDomainAccessControllerSubscriptionPublisher.fireOwnerAccessControlEntryChanged(
                            job.getChangeType(),
                            job.getOwnerAccessControlEntry(),
                            sanitizeForPartition(job.getOwnerAccessControlEntry().getUid()),
                            sanitizeForPartition(job.getOwnerAccessControlEntry().getDomain()),
                            sanitizeForPartition(job.getOwnerAccessControlEntry().getInterfaceName()));
                    // TODO: add
                    // sanitizeForPartition(job.getMasterAccessControlEntry().getOperation())
                } else if (job.getMasterRegistrationControlEntry() != null) {
                    globalDomainAccessControllerSubscriptionPublisher.fireMasterRegistrationControlEntryChanged(
                            job.getChangeType(),
                            job.getMasterRegistrationControlEntry(),
                            sanitizeForPartition(job.getMasterRegistrationControlEntry().getUid()),
                            sanitizeForPartition(job.getMasterRegistrationControlEntry().getDomain()),
                            sanitizeForPartition(job.getMasterRegistrationControlEntry().getInterfaceName()));
                } else if (job.getMediatorRegistrationControlEntry() != null) {
                    globalDomainAccessControllerSubscriptionPublisher.fireMediatorRegistrationControlEntryChanged(
                            job.getChangeType(),
                            job.getMediatorRegistrationControlEntry(),
                            sanitizeForPartition(job.getMediatorRegistrationControlEntry().getUid()),
                            sanitizeForPartition(job.getMediatorRegistrationControlEntry().getDomain()),
                            sanitizeForPartition(job.getMediatorRegistrationControlEntry().getInterfaceName()));
                } else if (job.getOwnerRegistrationControlEntry() != null) {
                    globalDomainAccessControllerSubscriptionPublisher.fireOwnerRegistrationControlEntryChanged(
                            job.getChangeType(),
                            job.getOwnerRegistrationControlEntry(),
                            sanitizeForPartition(job.getOwnerRegistrationControlEntry().getUid()),
                            sanitizeForPartition(job.getOwnerRegistrationControlEntry().getDomain()),
                            sanitizeForPartition(job.getOwnerRegistrationControlEntry().getInterfaceName()));
                }
            } catch (InterruptedException e) {
                logger.warn("cannot fire broadcast because no matching criteria present");
            }
        }
    }
}
