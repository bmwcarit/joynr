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

import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

/**
 * Reflects a {@link BounceProxyInformation} in a relational database.
 * 
 * @author christina.strobel
 *
 */
@Entity
public class BounceProxyInformationEntity {

    @Id
    private String id;

    private String clusterId;
    private String instanceId;
    private String urlForCc;
    private String urlForBpc;

    public BounceProxyInformationEntity() {

    }

    public BounceProxyInformationEntity(BounceProxyInformation info) {
        this.id = info.getId();
        this.urlForCc = info.getLocation().toString();
        this.urlForBpc = info.getLocation().toString();

        if (info instanceof ControlledBounceProxyInformation) {
            ControlledBounceProxyInformation cbpInfo = (ControlledBounceProxyInformation) info;
            this.clusterId = cbpInfo.getClusterId();
            this.instanceId = cbpInfo.getInstanceId();
            this.urlForBpc = cbpInfo.getLocationForBpc().toString();
        }

    }

    public BounceProxyInformationEntity(ControlledBounceProxyInformation info) {
        this.id = info.getId();
        this.clusterId = info.getClusterId();
        this.instanceId = info.getInstanceId();
        this.urlForCc = info.getLocation().toString();
        this.urlForBpc = info.getLocationForBpc().toString();
    }

    public String getClusterId() {
        return clusterId;
    }

    public void setClusterId(String clusterId) {
        this.clusterId = clusterId;
    }

    public String getInstanceId() {
        return instanceId;
    }

    public void setInstanceId(String instanceId) {
        this.instanceId = instanceId;
    }

    public String getUrlForCc() {
        return urlForCc;
    }

    public void setUrlForCc(String urlForCc) {
        this.urlForCc = urlForCc;
    }

    public String getUrlForBpc() {
        return urlForBpc;
    }

    public void setUrlForBpc(String urlForBpc) {
        this.urlForBpc = urlForBpc;
    }

    public ControlledBounceProxyInformation convertToBounceProxyInformation() {
        return new ControlledBounceProxyInformation(this.clusterId,
                                                    this.instanceId,
                                                    URI.create(this.urlForCc),
                                                    URI.create(this.urlForBpc));
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((clusterId == null) ? 0 : clusterId.hashCode());
        result = prime * result + ((id == null) ? 0 : id.hashCode());
        result = prime * result + ((instanceId == null) ? 0 : instanceId.hashCode());
        result = prime * result + ((urlForBpc == null) ? 0 : urlForBpc.hashCode());
        result = prime * result + ((urlForCc == null) ? 0 : urlForCc.hashCode());
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
        BounceProxyInformationEntity other = (BounceProxyInformationEntity) obj;
        if (clusterId == null) {
            if (other.clusterId != null)
                return false;
        } else if (!clusterId.equals(other.clusterId))
            return false;
        if (id == null) {
            if (other.id != null)
                return false;
        } else if (!id.equals(other.id))
            return false;
        if (instanceId == null) {
            if (other.instanceId != null)
                return false;
        } else if (!instanceId.equals(other.instanceId))
            return false;
        if (urlForBpc == null) {
            if (other.urlForBpc != null)
                return false;
        } else if (!urlForBpc.equals(other.urlForBpc))
            return false;
        if (urlForCc == null) {
            if (other.urlForCc != null)
                return false;
        } else if (!urlForCc.equals(other.urlForCc))
            return false;
        return true;
    }

}
