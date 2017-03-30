package io.joynr.messaging.bounceproxy.controller.directory.inmemory;

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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.system.TimestampProvider;

import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

import com.google.common.base.Predicate;
import com.google.common.collect.Collections2;
import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * Directory which stores all registered bounce proxies in memory.
 * 
 * @author christina.strobel
 * 
 */
@Singleton
public class InMemoryBounceProxyDirectory implements BounceProxyDirectory {

    @Inject
    private TimestampProvider timestampProvider;

    /**
     * Hashmap of bounce proxies by bounce proxy IDs.
     */
    private HashMap<String, BounceProxyRecord> directory = new HashMap<String, BounceProxyRecord>();

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory
     * #getAssignableBounceProxies()
     */
    @Override
    public List<BounceProxyRecord> getAssignableBounceProxies() {

        Predicate<BounceProxyRecord> statusIsAssignablePredicate = new Predicate<BounceProxyRecord>() {

            @Override
            public boolean apply(BounceProxyRecord record) {
                if (record == null)
                    return false;
                return record.getStatus().isAssignable();
            }
        };
        Collection<BounceProxyRecord> assignableBounceProxies = Collections2.filter(directory.values(),
                                                                                    statusIsAssignablePredicate);

        return new LinkedList<BounceProxyRecord>(assignableBounceProxies);
    }

    @Override
    public void updateChannelAssignment(String ccid, BounceProxyInformation bpInfo) {

        if (!directory.containsKey(bpInfo.getId()))
            throw new IllegalArgumentException("No bounce proxy with ID '" + bpInfo.getId()
                    + "' available in the directory");

        BounceProxyRecord record = directory.get(bpInfo.getId());
        record.addAssignedChannel(ccid);
        record.setLastAssignedTimestamp(timestampProvider.getCurrentTime());
    }

    @Override
    public BounceProxyRecord getBounceProxy(String bpId) {

        if (!directory.containsKey(bpId))
            throw new IllegalArgumentException("No bounce proxy with ID '" + bpId + "' available in the directory");

        return directory.get(bpId);
    }

    @Override
    public void addBounceProxy(ControlledBounceProxyInformation bpInfo) {

        if (directory.containsKey(bpInfo.getId()))
            throw new IllegalArgumentException("Bounce proxy with ID '" + bpInfo.getId()
                    + "' already added to the directory");

        BounceProxyRecord record = new BounceProxyRecord(bpInfo);
        record.setFreshness(timestampProvider.getCurrentTime());
        directory.put(bpInfo.getId(), record);
    }

    @Override
    public List<BounceProxyStatusInformation> getBounceProxyStatusInformation() {
        return new LinkedList<BounceProxyStatusInformation>(directory.values());
    }

    @Override
    public boolean containsBounceProxy(String bpId) {
        return directory.containsKey(bpId);
    }

    @Override
    public void updateBounceProxy(BounceProxyRecord bpRecord) throws IllegalArgumentException {
        if (!directory.containsKey(bpRecord.getInfo().getId()))
            throw new IllegalArgumentException("No bounce proxy with ID '" + bpRecord.getInfo().getId()
                    + "' available in the directory");

        bpRecord.setFreshness(timestampProvider.getCurrentTime());
        directory.put(bpRecord.getInfo().getId(), bpRecord);
    }

}
