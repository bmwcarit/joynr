package io.joynr.integration.setup;

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

import io.joynr.integration.ControlledBounceProxyServerTest;
import io.joynr.integration.util.ServersUtil;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Setup for a single controlled bounce proxy and a bounce proxy controller.
 */
public class SingleControlledBounceProxy implements BounceProxyServerSetup {

    private static final Logger logger = LoggerFactory.getLogger(ControlledBounceProxyServerTest.class);

    public static final String ID = "X.Y";

    private Server bounceProxyServerXY;
    private Server bounceProxyControllerServer;

    private String bounceProxyServerXyUrl;
    private String bounceProxyControllerUrl;

    @Override
    public void startServers() throws Exception {
        // start different servers to make sure that handling of different URLs
        // works
        logger.info("Starting Bounceproxy Controller");
        bounceProxyControllerServer = ServersUtil.startBounceproxyController();

        logger.info("Starting Controlled Bounceproxy X.Y");
        bounceProxyServerXY = ServersUtil.startControlledBounceproxy(ID);

        logger.info("All servers started");

        int bounceProxyServerXyPort = ((ServerConnector) bounceProxyServerXY.getConnectors()[0]).getPort();
        bounceProxyServerXyUrl = "http://localhost:" + bounceProxyServerXyPort + "/bounceproxy/";

        int bounceProxyControllerPort = ((ServerConnector) bounceProxyControllerServer.getConnectors()[0]).getPort();
        bounceProxyControllerUrl = "http://localhost:" + bounceProxyControllerPort + "/controller/";
    }

    @Override
    public void stopServers() throws Exception {
        bounceProxyServerXY.stop();
        bounceProxyControllerServer.stop();
    }

    @Override
    public String getAnyBounceProxyUrl() {
        return bounceProxyServerXyUrl;
    }

    @Override
    public String getBounceProxyControllerUrl() {
        return bounceProxyControllerUrl;
    }

    @Override
    public String getBounceProxyUrl(String bpId) {
        if (bpId.equals(ID))
            return bounceProxyServerXyUrl;

        throw new IllegalArgumentException("No Bounce Proxy with ID '" + bpId + "' in existing configuration");
    }

}
