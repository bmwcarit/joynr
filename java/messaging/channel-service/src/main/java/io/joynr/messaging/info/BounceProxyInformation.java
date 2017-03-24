package io.joynr.messaging.info;

/*
 * #%L
 * joynr::java::messaging::channel-service
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

import java.io.Serializable;
import java.net.URI;

/**
 * Holds information about a bounce proxy. E.g. the URL that cluster controllers
 * can use to communicate with the bounce proxy.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyInformation implements Serializable {
    private static final long serialVersionUID = 6615249371062916830L;

    /**
     * The location of the bounce proxy at which it is reachable for cluster
     * controllers.
     */
    private URI location;

    /**
     * The identifier of the bounce proxy.
     */
    private String id;

    public BounceProxyInformation(String id, URI location) {
        this.id = id;
        this.location = location;
    }

    /**
     * Sets the location of the bounce proxy at which it is reachable for
     * cluster controllers.
     * 
     * @param location
     *   URI of the bounce proxy which is reachable for
     *   cluster controllers
     */
    public void setLocation(URI location) {
        this.location = location;
    }

    /**
     * Gets the location of the bounce proxy at which it is reachable for
     * cluster controllers.
     * 
     * @return location as URI
     */
    public URI getLocation() {
        return location;
    }

    /**
     * Gets the identifier of the bounce proxy.
     * 
     * @return an identifier
     */
    public String getId() {
        return id;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((id == null) ? 0 : id.hashCode());
        result = prime * result + ((location == null) ? 0 : location.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {

        if (this == obj) {
            return true;
        }

        // returns false if obj is null
        if (!(obj instanceof BounceProxyInformation)) {
            return false;
        }

        BounceProxyInformation bpInfo = (BounceProxyInformation) obj;

        return bpInfo.id.equals(id) && bpInfo.location.equals(location);
    }

}
