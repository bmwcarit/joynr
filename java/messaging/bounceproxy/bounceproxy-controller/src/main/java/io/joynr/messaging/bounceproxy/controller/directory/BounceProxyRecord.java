package io.joynr.messaging.bounceproxy.controller.directory;

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

import io.joynr.messaging.bounceproxy.controller.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;

/**
 * Database record for a bounce proxy instance.
 * 
 * @author christina.strobel
 *
 */
public class BounceProxyRecord {

    public static final long ASSIGNMENT_TIMESTAMP_NEVER = -1;

    private ControlledBounceProxyInformation info;
    private BounceProxyStatus status;

    private int assignedChannels;

    private long lastAssignedTimestamp;

    public BounceProxyRecord(ControlledBounceProxyInformation bpInfo) {
        this.info = bpInfo;
        this.lastAssignedTimestamp = ASSIGNMENT_TIMESTAMP_NEVER;
        this.assignedChannels = 0;
        this.status = BounceProxyStatus.ACTIVE;
    }

    public ControlledBounceProxyInformation getInfo() {
        return info;
    }

    public void setInfo(ControlledBounceProxyInformation info) {
        this.info = info;
    }

    public BounceProxyStatus getStatus() {
        return status;
    }

    public void setStatus(BounceProxyStatus status) {
        this.status = status;
    }

    public int getAssignedChannels() {
        return assignedChannels;
    }

    public void setAssignedChannels(int assignedChannels) {
        this.assignedChannels = assignedChannels;
    }

    /**
     * Returns the timestamp of the latest assignment of a channel to that
     * bounce proxy instance.
     * 
     * @return a timestamp of the latest channel assignment or
     *         {@link #ASSIGNMENT_TIMESTAMP_NEVER} if no channel has ever been
     *         assigned.
     */
    public long getLastAssignedTimestamp() {
        return lastAssignedTimestamp;
    }

    public void setLastAssignedTimestamp(long lastAssignedTimestamp) {
        this.lastAssignedTimestamp = lastAssignedTimestamp;
    }

    /**
     * Increases the number of assigned channels and updates the timestamp of the latest channel assignment.
     */
    public void increaseAssignedChannels() {
        assignedChannels++;
        lastAssignedTimestamp = System.currentTimeMillis();
    }

}
