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
 * Interface to define a setup of bounce proxy servers.
 * 
 * The setup can consist of a standalone bounce proxy or any number of
 * controlled bounce proxies and a bounce proxy controller. Subclasses define
 * the setup.<br>
 * 
 * The configuration can be accessed by the test class by annotating a public
 * field with the
 * {@link io.joynr.integration.setup.testrunner.BounceProxyServerContext}
 * annotation: <code>
 * 
 * @Configuration public BounceProxyConfiguration configuration; <code>
 * 
 */
public interface BounceProxyServerSetup {

    /**
     * Starts all servers needed for the setup.
     * 
     * @throws Exception
     *             if one of the servers can't be started properly.
     */
    void startServers() throws Exception;

    /**
     * Stops all servers needed for the setup.
     * 
     * @throws Exception
     *             if one of the servers can't be stopped properly.
     */
    void stopServers() throws Exception;

    /**
     * Returns the URL of the bounce proxy controller or the single standalone
     * bounce proxy, respectively. This is the URL used for channel creation.
     * 
     * @return
     */
    String getBounceProxyControllerUrl();

    /**
     * Returns the URL of the single standalone bounce proxy or the URL of any
     * controlled bounce proxy in the setup.
     * 
     * @return
     */
    String getAnyBounceProxyUrl();

    /**
     * Returns the URL of the bounce proxy with the given ID.
     * 
     * @param bpId
     * @return
     * @throws IllegalArgumentException
     *             if no bounce proxy with the ID exists in the setup.
     */
    String getBounceProxyUrl(String bpId);
}
