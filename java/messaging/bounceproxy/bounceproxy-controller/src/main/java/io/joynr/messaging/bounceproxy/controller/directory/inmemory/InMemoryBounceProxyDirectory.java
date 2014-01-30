package io.joynr.messaging.bounceproxy.controller.directory.inmemory;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.bounceproxy.controller.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.BounceProxyStatusInformation;

import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

import com.google.common.base.Predicate;
import com.google.common.collect.Collections2;
import com.google.inject.Singleton;

/**
 * Directory which stores all registered bounce proxies in memory. 
 * 
 * @author christina.strobel
 * 
 */
@Singleton
public class InMemoryBounceProxyDirectory implements BounceProxyDirectory {

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

        Collection<BounceProxyRecord> assignableBounceProxies = Collections2.filter(directory.values(),
                                                                                    new Predicate<BounceProxyRecord>() {

                                                                                        @Override
                                                                                        public boolean apply(BounceProxyRecord record) {
                                                                                            if (record == null)
                                                                                                return false;
                                                                                            return record.getStatus()
                                                                                                         .isAssignable();
                                                                                        }
                                                                                    });

        return new LinkedList<BounceProxyRecord>(assignableBounceProxies);
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory
     * #updateChannelAssignment(java.lang.String,
     * io.joynr.messaging.info.BounceProxyInformation)
     */
    @Override
    public void updateChannelAssignment(String ccid, BounceProxyInformation bpInfo) {

        if (directory.containsKey(bpInfo.getId())) {
            BounceProxyRecord record = directory.get(bpInfo.getId());
            record.increaseAssignedChannels();
            record.setLastAssignedTimestamp(System.currentTimeMillis());
        }
    }

    @Override
    public BounceProxyRecord getBounceProxy(String bpId) {
        return directory.get(bpId);
    }

    @Override
    public void addBounceProxy(ControlledBounceProxyInformation bpInfo) {

        BounceProxyRecord record = new BounceProxyRecord(bpInfo);
        directory.put(bpInfo.getId(), record);
    }

    @Override
    public void updateBounceProxyStatus(String bpId, BounceProxyStatus status) {
        BounceProxyRecord record = directory.get(bpId);
        record.setStatus(status);
    }

    @Override
    public List<String> getBounceProxyIds() {
        return new LinkedList<String>(directory.keySet());
    }

    @Override
    public List<BounceProxyStatusInformation> getBounceProxyStatusInformation() {
        return new LinkedList<BounceProxyStatusInformation>(directory.values());
    }

}
