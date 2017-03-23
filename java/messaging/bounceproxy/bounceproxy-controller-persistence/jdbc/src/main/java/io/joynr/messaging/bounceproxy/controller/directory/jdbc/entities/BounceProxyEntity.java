package io.joynr.messaging.bounceproxy.controller.directory.jdbc.entities;

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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyRecord;
import io.joynr.messaging.info.BounceProxyStatus;

import java.util.HashSet;
import java.util.Set;

import javax.persistence.CascadeType;
import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.OneToMany;
import javax.persistence.OneToOne;

/**
 * Reflects a {@link BounceProxyRecord} in a relational database.
 * 
 * @author christina.strobel
 *
 */
@Entity
public class BounceProxyEntity {

    @Id
    private String bpId;

    private long freshness;
    private long lastAssignedTimestamp;

    private String status;

    @OneToOne(cascade = CascadeType.ALL)
    private BounceProxyInformationEntity bpInfo;

    @OneToMany(cascade = CascadeType.ALL)
    private Set<ChannelEntity> channels;

    public BounceProxyEntity() {

    }

    public BounceProxyEntity(BounceProxyRecord bpRecord) {

        this.bpId = bpRecord.getBounceProxyId();

        this.freshness = bpRecord.getFreshness().getTime();
        this.lastAssignedTimestamp = bpRecord.getLastAssignedTimestamp();

        this.bpInfo = new BounceProxyInformationEntity(bpRecord.getInfo());

        this.status = bpRecord.getStatus().name();

        this.channels = new HashSet<ChannelEntity>();
    }

    public String getBpId() {
        return bpId;
    }

    public void setBpId(String bpId) {
        this.bpId = bpId;
    }

    public long getFreshness() {
        return freshness;
    }

    public void setFreshness(long freshness) {
        this.freshness = freshness;
    }

    public long getLastAssignedTimestamp() {
        return lastAssignedTimestamp;
    }

    public void setLastAssignedTimestamp(long lastAssignedTimestamp) {
        this.lastAssignedTimestamp = lastAssignedTimestamp;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public BounceProxyInformationEntity getBpInfo() {
        return bpInfo;
    }

    public void setBpInfo(BounceProxyInformationEntity bpInfo) {
        this.bpInfo = bpInfo;
    }

    public Set<ChannelEntity> getChannels() {
        return channels;
    }

    public void setChannels(Set<ChannelEntity> channels) {
        this.channels = channels;
    }

    public BounceProxyRecord convertToBounceProxyRecord() {

        BounceProxyRecord bpRecord = new BounceProxyRecord(bpInfo.convertToBounceProxyInformation());

        for (ChannelEntity channelEntity : this.channels) {
            bpRecord.addAssignedChannel(channelEntity.getChannelId());
        }

        bpRecord.setFreshness(this.freshness);
        bpRecord.setLastAssignedTimestamp(this.lastAssignedTimestamp);
        bpRecord.setStatus(BounceProxyStatus.valueOf(this.status));

        return bpRecord;
    }

    /**
     * Adds a channel to the list of channels assigned to the bounce proxy.
     * 
     * If a channel with the channel ID already exists in the set of assigned
     * channels, the element isn't added.
     * 
     * @param channelEntity Channel description
     */
    public void addChannel(ChannelEntity channelEntity) {
        channels.add(channelEntity);
    }
}
