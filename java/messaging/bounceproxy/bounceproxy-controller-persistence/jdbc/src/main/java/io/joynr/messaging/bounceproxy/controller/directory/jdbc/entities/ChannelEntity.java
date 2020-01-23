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
package io.joynr.messaging.bounceproxy.controller.directory.jdbc.entities;

import java.net.URI;

import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.ManyToOne;

import io.joynr.messaging.info.Channel;

/**
 * Reflects a {@link Channel} in a relational database.
 * 
 * @author christina.strobel
 *
 */
@Entity
public class ChannelEntity {

    @Id
    private String channelId;

    private String location;

    @ManyToOne
    private BounceProxyInformationEntity bpInfo;

    public ChannelEntity() {
        this.channelId = "";
        this.location = "";
        this.bpInfo = null;
    }

    public ChannelEntity(String channelId, BounceProxyInformationEntity info, String location) {
        this.channelId = channelId;
        this.bpInfo = info;
        this.location = location;
    }

    public String getChannelId() {
        return channelId;
    }

    public void setChannelId(String channelId) {
        this.channelId = channelId;
    }

    public String getLocation() {
        return location;
    }

    public void setLocation(String location) {
        this.location = location;
    }

    public BounceProxyInformationEntity getBpInfo() {
        return bpInfo;
    }

    public void setBpInfo(BounceProxyInformationEntity bpInfo) {
        this.bpInfo = bpInfo;
    }

    public Channel convertToChannel() {
        return new Channel(bpInfo.convertToBounceProxyInformation(), this.channelId, URI.create(this.location));
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((bpInfo == null) ? 0 : bpInfo.hashCode());
        result = prime * result + ((channelId == null) ? 0 : channelId.hashCode());
        result = prime * result + ((location == null) ? 0 : location.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        ChannelEntity other = (ChannelEntity) obj;
        if (bpInfo == null) {
            if (other.bpInfo != null)
                return false;
        } else if (!bpInfo.equals(other.bpInfo))
            return false;
        if (channelId == null) {
            if (other.channelId != null)
                return false;
        } else if (!channelId.equals(other.channelId))
            return false;
        if (location == null) {
            if (other.location != null)
                return false;
        } else if (!location.equals(other.location))
            return false;
        return true;
    }

}
