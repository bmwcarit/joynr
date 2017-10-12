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
package io.joynr.integration.setup;

/**
 * Representation of a node or server instance, respectively, in a cluster.<br>
 * This class contains all configuration parameters specific to that node, no
 * matter to which web application they belong.
 * 
 * @author christina.strobel
 * 
 */
public class ClusterNode {

    private String contextPath;
    private String ehcacheConfigFile;
    private int instanceId;

    public ClusterNode(int instanceId, String contextPath, String ehcacheConfigFile) {
        this.instanceId = instanceId;
        this.contextPath = contextPath;
        this.ehcacheConfigFile = ehcacheConfigFile;
    }

    /**
     * Returns the context path that represents the server instance.
     * 
     * @return
     */
    public String getContextPath() {
        return this.contextPath;
    }

    /**
     * Returns the configuration file used for this server instance to
     * synchronize the state of bounceproxy controller and bounceproxies.
     * 
     * @return
     */
    public String getEhcacheConfigFile() {
        return this.ehcacheConfigFile;
    }

    /**
     * The identifier of the bounceproxy associated with this server instance.
     * 
     * @return
     */
    public String getBounceProxyId() {
        return "bounceproxy." + contextPath;
    }

    /**
     * An instance number for the server instance.
     * 
     * @return
     */
    public int getInstanceId() {
        return this.instanceId;
    }
}
