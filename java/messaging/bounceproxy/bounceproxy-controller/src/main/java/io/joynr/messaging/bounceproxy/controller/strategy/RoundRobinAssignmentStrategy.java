package io.joynr.messaging.bounceproxy.controller.strategy;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.exceptions.JoynrChannelNotAssignableException;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import com.google.inject.Inject;

/**
 * Assignment strategy to simply assign the channels round-robin.
 *
 * @author christina.strobel
 *
 */
public class RoundRobinAssignmentStrategy implements ChannelAssignmentStrategy {

    @Inject
    private BounceProxyDirectory bpDirectory;

    /*
     * (non-Javadoc)
     *
     * @see
     * io.joynr.messaging.bounceproxy.controller.strategy.ChannelAssignmentStrategy
     * #calculateBounceProxy(java.lang.String)
     */
    @Override
    public ControlledBounceProxyInformation calculateBounceProxy(String ccid) {

        // get all available bounce proxies from the list
        List<BounceProxyRecord> records = bpDirectory.getAssignableBounceProxies();

        if (records == null || records.isEmpty()) {
            throw new JoynrChannelNotAssignableException("No bounce proxy instances available", ccid);
        }

        // Sort records by the timestamp when they were last assigned a channel.
        // Smallest timestamp should appear first in the list.
        // Records that never have been assigned a channel have the timestamp
        // -1, so they appear first in the list.
        Collections.sort(records, new Comparator<BounceProxyRecord>() {

            @Override
            public int compare(BounceProxyRecord o1, BounceProxyRecord o2) {

                if (o1.getLastAssignedTimestamp() == o2.getLastAssignedTimestamp()) {
                    return 0;
                }

                if (o1.getLastAssignedTimestamp() == BounceProxyRecord.ASSIGNMENT_TIMESTAMP_NEVER) {
                    return -1;
                }

                if (o2.getLastAssignedTimestamp() == BounceProxyRecord.ASSIGNMENT_TIMESTAMP_NEVER) {
                    return 1;
                }

                return (int) (o1.getLastAssignedTimestamp() - o2.getLastAssignedTimestamp());
            }
        });

        // find the first one in the list which is the next to assign a channel
        // to
        return records.get(0).getInfo();
    }
}
