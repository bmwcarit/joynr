package io.joynr.messaging.info;

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

import java.net.URI;

/**
 * Contains information for a bounce proxy that is controlled by a bounce proxy
 * controller. This includes information about the cluster the bounce proxy is
 * running on.
 * 
 * @author christina.strobel
 * 
 */
public class ControlledBounceProxyInformation extends BounceProxyInformation {
    private static final long serialVersionUID = 244924388532995344L;

    private String instanceId;
    private String clusterId;
    private URI locationForBpc;

    /**
     * Creates new bounce proxy information from a bounce proxy ID and a
     * location.
     * 
     * @param bpId
     *   bounce proxy ID in the format {@literal <clusterId><instanceId>}
     * @param location
     *   bounce proxy url for both cluster controllers and bounce proxy
     *   controllers
     * @throws IllegalArgumentException
     *   if bounce proxy ID has the wrong format
     */
    public ControlledBounceProxyInformation(String bpId, URI location) {
        this(bpId, location, location);
    }

    /**
     * Creates new bounce proxy information from a bounce proxy ID and separate
     * locations for cluster controllers and bounce proxy controllers.
     * 
     * @param bpId
     *   bounce proxy ID in the format {@literal <clusterId><instanceId>}
     * @param locationForCCs
     *   bounce proxy url for cluster controllers that are somewhere on
     *   the internet
     * @param locationForBpc
     *   bounce proxy url for bounce proxy controller that is possibly
     *   on the same subnet
     * @throws IllegalArgumentException
     *   if bounce proxy ID has the wrong format
     */
    public ControlledBounceProxyInformation(String bpId, URI locationForCCs, URI locationForBpc) {
        super(bpId, locationForCCs);

        String[] bpIdTokens = bpId.split("\\.");

        if (bpIdTokens.length != 2) {
            throw new IllegalArgumentException("Bounce proxy ID expected in the format <clusterId>.<instanceID>, but was :"
                    + bpId);
        }

        this.instanceId = bpIdTokens[1];
        this.clusterId = bpIdTokens[0];
        this.locationForBpc = locationForBpc;
    }

    /**
     * Creates new bounce proxy information from a cluster ID, instance ID and a
     * location.
     * 
     * @param clusterId
     *   the id of the cluster
     * @param instanceId
     *   the id of the instance
     * @param location
     *   bounce proxy url for both cluster controllers and bounce proxy
     *   controllers
     */
    public ControlledBounceProxyInformation(String clusterId, String instanceId, URI location) {
        this(clusterId, instanceId, location, location);
    }

    /**
     * @param clusterId
     *   the id of the cluster
     * @param instanceId
     *   the id of the instance
     * @param locationForCCs
     *   bounce proxy url for cluster controllers that are somewhere on
     *   the internet
     * @param locationForBpc
     *   bounce proxy url for bounce proxy controller that is possibly
     *   on the same subnet
     */
    public ControlledBounceProxyInformation(String clusterId, String instanceId, URI locationForCCs, URI locationForBpc) {
        super(clusterId + "." + instanceId, locationForCCs);
        this.locationForBpc = locationForBpc;
        this.instanceId = instanceId;
        this.clusterId = clusterId;
    }

    /**
     * Returns the identifier of the bounce proxy instance.
     * 
     * @return the id of the bounce proxy instance
     */
    public String getInstanceId() {
        return this.instanceId;
    }

    /**
     * Returns the identifier of the server cluster that the bounce proxy is
     * running on.
     * 
     * @return the cluster id
     */
    public String getClusterId() {
        return clusterId;
    }

    /**
     * Sets the location at which the bounce proxy is reachable for the bounce
     * proxy controller.
     * 
     * @param locationForBpc
     *            bounce proxy url for bounce proxy controller that is possibly
     *            on the same subnet
     */
    public void setLocationForBpc(URI locationForBpc) {
        this.locationForBpc = locationForBpc;
    }

    /**
     * Returns the location for bounce proxy controllers.
     * 
     * @return the location for bounce proxy controllers
     */
    public URI getLocationForBpc() {
        return locationForBpc;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((clusterId == null) ? 0 : clusterId.hashCode());
        result = prime * result + ((instanceId == null) ? 0 : instanceId.hashCode());
        result = prime * result + ((locationForBpc == null) ? 0 : locationForBpc.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {

        if (this == obj) {
            return true;
        }

        if (!super.equals(obj)) {
            return false;
        }

        // returns false if obj is null
        if (!(obj instanceof ControlledBounceProxyInformation)) {
            return false;
        }

        ControlledBounceProxyInformation bpInfo = (ControlledBounceProxyInformation) obj;
        return bpInfo.getInstanceId().equals(instanceId) && bpInfo.clusterId.equals(clusterId)
                && bpInfo.locationForBpc.equals(locationForBpc);
    }

}
