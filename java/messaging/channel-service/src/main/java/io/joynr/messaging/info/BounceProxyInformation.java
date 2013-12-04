package io.joynr.messaging.info;

/*
 * #%L
 * joynr::java::messaging::channel-service
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

import java.net.URI;

/**
 * Holds information about a bounce proxy. E.g. the URL that cluster controllers
 * can use to communicate with the bounce proxy.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyInformation {

    private URI location;
    private String instanceId;
    private String clusterId;

    public BounceProxyInformation(String clusterId, String instanceId, URI location) {

        if (!location.toString().endsWith("/")) {
            // for URI resolution, the URI has to end with a /
            this.location = URI.create(location.toString() + "/");
        } else {
            this.location = location;
        }

        this.instanceId = instanceId;
        this.clusterId = clusterId;
    }

    /**
     * Gets the location of the bounce proxy.
     * 
     * @return location as URI
     */
    public URI getLocation() {
        return location;
    }

    /**
     * Gets the identifier of the bounce proxy, in the format "clusterId.instanceId".
     * 
     * @return an identifier
     */
    public String getId() {
        return clusterId + "." + instanceId;
    }

    /**
     * Returns the identifier of the bounce proxy instance only, without 
     * 
     * @return
     */
    public String getInstanceId() {
        return instanceId;
    }

}
