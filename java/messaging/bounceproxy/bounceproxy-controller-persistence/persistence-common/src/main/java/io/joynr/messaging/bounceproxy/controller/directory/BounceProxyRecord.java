package io.joynr.messaging.bounceproxy.controller.directory;

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

import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.BounceProxyStatusInformation;
import io.joynr.messaging.info.PerformanceMeasures;

import java.io.Serializable;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;

/**
 * Database record for a bounce proxy instance.<br>
 * This class does not contain any logic but only sets and gets attributes, as
 * this only reflects an entry of a database.
 *
 * @author christina.strobel
 *
 */
public class BounceProxyRecord implements BounceProxyStatusInformation, Serializable {
    private static final long serialVersionUID = 3882680863826856386L;

    public static final long ASSIGNMENT_TIMESTAMP_NEVER = -1;

    private ControlledBounceProxyInformation info;
    private BounceProxyStatus status;
    /**
     * The performance measures as sent by the bounce proxy.
     */
    private PerformanceMeasures performanceMeasures;

    /**
     * The freshness of the record, i.e. the timestamp when this record was
     * marked as up to date.
     */
    private long freshness;

    /**
     * A set of assigned channels as recorded by the bounce proxy
     * controller. The size of this set should match with the number of assigned channels
     * reported by the bounce proxy in {@link #performanceMeasures}.
     */
    private Set<String> assignedChannels;

    private long lastAssignedTimestamp;

    public BounceProxyRecord(ControlledBounceProxyInformation bpInfo) {
        this.info = bpInfo;
        this.lastAssignedTimestamp = ASSIGNMENT_TIMESTAMP_NEVER;
        this.assignedChannels = new HashSet<String>();
        this.status = BounceProxyStatus.ALIVE;
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

    /**
     * Sets the status of the bounce proxy.
     *
     * @param status the status of the bounce proxy
     * @throws IllegalStateException
     *             if setting this status is not possible for the current bounce
     *             proxy status.
     */
    public void setStatus(BounceProxyStatus status) throws IllegalStateException {
        // this checks if the transition is valid
        if (this.status.isValidTransition(status)) {
            this.status = status;
        } else {
            throw new IllegalStateException("Illegal status transition from " + this.status + " to " + status);
        }
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
     * Adds an assigned channel if it doesn't exist yet. The timestamp of the latest
     * channel assignment has to be updated manually.
     *
     * @param channelId the ID of the channel to be added
     */
    public void addAssignedChannel(String channelId) {
        assignedChannels.add(channelId);
    }

    /**
     * Returns the number of channels assigned to this bounce proxy.
     *
     * @return number of channels assigned to this bounce proxy
     */
    public int getNumberOfAssignedChannels() {
        return assignedChannels.size();
    }

    @Override
    public String getBounceProxyId() {
        return info.getId();
    }

    @Override
    public Date getFreshness() {
        return new Date(freshness);
    }

    @Override
    public PerformanceMeasures getPerformanceMeasures() {
        return this.performanceMeasures;
    }

    /**
     * Sets the performance measures for this records as they were sent by the
     * bounce proxy.
     *
     * @param performanceMeasures
     *            includes map of performance measures for bounce proxies
     */
    public void setPerformanceMeasures(PerformanceMeasures performanceMeasures) {
        this.performanceMeasures = performanceMeasures;
    }

    /**
     * Updates the freshness of the record.
     *
     * @param timestamp
     *            timestamp
     */
    public void setFreshness(long timestamp) {
        this.freshness = timestamp;
    }
}
