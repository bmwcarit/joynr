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

import org.eclipse.jetty.server.Server;

import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.MessagingPropertyKeys;

/**
 * Setup for a single bounce proxy.
 */
public class SingleBounceProxy implements BounceProxyServerSetup {

    private Server bounceproxyServer;
    private String bounceProxyUrl;

    @Override
    public void startServers() throws Exception {
        bounceproxyServer = ServersUtil.startBounceproxy();
        bounceProxyUrl = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);
    }

    @Override
    public void stopServers() throws Exception {
        bounceproxyServer.stop();
    }

    @Override
    public String getBounceProxyControllerUrl() {
        return bounceProxyUrl;
    }

    @Override
    public String getAnyBounceProxyUrl() {
        return bounceProxyUrl;
    }

    @Override
    public String getBounceProxyUrl(String bpId) {
        return bounceProxyUrl;
    }

}
