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
import org.eclipse.jetty.server.ServerConnector;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.integration.ControlledBounceProxyServerTest;
import io.joynr.integration.util.ServersUtil;

/**
 * Setup for a bounce proxy cluster of two bounce proxy instances and a bounce
 * proxy controller.
 * 
 * @author christina.strobel
 * 
 */
public class ControlledBounceProxyCluster implements BounceProxyServerSetup {

    private static final Logger logger = LoggerFactory.getLogger(ControlledBounceProxyServerTest.class);

    private Server bounceProxyServerXY;
    private Server bounceProxyServerAB;
    private Server bounceProxyControllerServer;

    private String bounceProxyServerXyUrl;
    private String bounceProxyServerAbUrl;
    private String bounceProxyControllerUrl;

    @Override
    public void startServers() throws Exception {
        // start different servers to make sure that handling of different URLs
        // works
        logger.info("Starting Bounceproxy Controller");
        bounceProxyControllerServer = ServersUtil.startBounceproxyController();
        int bounceProxyControllerPort = ((ServerConnector) bounceProxyControllerServer.getConnectors()[0]).getPort();
        bounceProxyControllerUrl = "http://localhost:" + bounceProxyControllerPort + "/controller/";

        logger.info("Starting Controlled Bounceproxy X.Y");
        bounceProxyServerXY = ServersUtil.startControlledBounceproxy("X.Y");

        logger.info("Starting Controlled Bounceproxy A.B");
        bounceProxyServerAB = ServersUtil.startControlledBounceproxy("A.B");

        logger.info("All servers started");

        int bounceProxyServerXyPort = ((ServerConnector) bounceProxyServerXY.getConnectors()[0]).getPort();
        bounceProxyServerXyUrl = "http://localhost:" + bounceProxyServerXyPort + "/bounceproxy/";

        int bounceProxyServerAbPort = ((ServerConnector) bounceProxyServerAB.getConnectors()[0]).getPort();
        bounceProxyServerAbUrl = "http://localhost:" + bounceProxyServerAbPort + "/bounceproxy/";
    }

    @Override
    public void stopServers() throws Exception {
        bounceProxyServerXY.stop();
        bounceProxyServerAB.stop();
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
        if (bpId.equals("X.Y"))
            return bounceProxyServerXyUrl;

        if (bpId.equals("A.B"))
            return bounceProxyServerAbUrl;

        throw new IllegalArgumentException("No Bounce Proxy with ID '" + bpId + "' in existing configuration");
    }
}
