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
import io.joynr.servlet.ServletUtil;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.ContextHandlerCollection;
import org.eclipse.jetty.webapp.WebAppContext;
import org.junit.Assert;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Starts a certain number of server instances that contain a bounceproxy and a
 * bounceproxy controller each, as well as a single request dispatcher.<br>
 * Clients send their requests to the dispatcher only. The dispatcher forwards
 * their requests to either the bounceproxy or the bounceproxy controller,
 * depending on the request path.<br>
 * For more detailed information on how requests are dispatched, see {@link TestRequestDispatcher}.
 *
 * To be able to use built-in forwarding mechanisms, all so-called server
 * instances run on the same Jetty server. Dispatcher and each server instance
 * are represented by their own context (e.g. /dispatcher, /instance0,
 * /instance1, ...). In each "instance" context, both a bounceproxy and a
 * bounceproxy controller are deployed, i.e. /instance0/controller and
 * /instance0/bounceproxy.
 *
 * @author christina.strobel
 *
 */
public class ClusteredBounceProxyWithDispatcher implements BounceProxyServerSetup {

    static final Logger logger = LoggerFactory.getLogger(ControlledBounceProxyServerTest.class);

    private static final String SL = "/";

    private static final String CONTEXT_BOUNCEPROXY = "bounceproxy";
    private static final String CONTEXT_CONTROLLER = "controller";
    static final String CONTEXT_DISPATCHER = "dispatcher";
    private static final String BP_CACHE_NAME = "bpCache1";

    private Server server;

    private String serverBaseUrl;
    private String bpcDispatcherUrl;

    private List<WebAppContext> bounceProxyContexts;

    private final HashMap<Integer, ClusterNode> serverInstances;

    public ClusteredBounceProxyWithDispatcher() {
        serverInstances = new HashMap<Integer, ClusterNode>();

        serverInstances.put(0, new ClusterNode(0, "instance0", "ehcache_distributed1.xml"));
        serverInstances.put(1, new ClusterNode(1, "instance1", "ehcache_distributed2.xml"));
    }

    @Override
    public void startServers() throws Exception {

        // start all in one server to allow simple dispatching
        int port = ServletUtil.findFreePort();
        server = new Server(port);

        serverBaseUrl = String.format("http://localhost:%d/", port);
        bpcDispatcherUrl = serverBaseUrl + CONTEXT_DISPATCHER + SL + CONTEXT_CONTROLLER + SL;

        server.setHandler(createWebAppContexts());
        server.start();

        waitForBounceProxyRegistrationAtAllControllers(10000l);
    }

    @Override
    public void stopServers() throws Exception {

        // stop bounceproxies first as they want to de-register
        for (WebAppContext bpContext : bounceProxyContexts) {
            bpContext.stop();
        }

        Thread.sleep(1000l);
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
    }

    @Override
    public String getAnyBounceProxyUrl() {
        return bpcDispatcherUrl;
    }

    @Override
    public String getBounceProxyControllerUrl() {
        return bpcDispatcherUrl;
    }

    @Override
    public String getBounceProxyUrl(String bpId) {

        for (ClusterNode instance : serverInstances.values()) {
            if (instance.getBounceProxyId().equals(bpId)) {
                return serverBaseUrl + instance.getContextPath() + SL + CONTEXT_BOUNCEPROXY;
            }
        }

        throw new IllegalArgumentException("No Bounce Proxy with ID '" + bpId + "' in existing configuration");
    }

    private ContextHandlerCollection createWebAppContexts() {

        ContextHandlerCollection handlers = new ContextHandlerCollection();

        addDispatcherContext(handlers);
        addBounceProxyControllerContexts(handlers);
        addBounceProxyContexts(handlers);
        return handlers;
    }

    private void addBounceProxyContexts(ContextHandlerCollection handlers) {
        bounceProxyContexts = new LinkedList<WebAppContext>();
        for (ClusterNode instance : serverInstances.values()) {
            WebAppContext bpContext = createBpContext(instance);
            bounceProxyContexts.add(bpContext);
            handlers.addHandler(bpContext);
        }
    }

    private void addBounceProxyControllerContexts(ContextHandlerCollection handlers) {
        for (ClusterNode instance : serverInstances.values()) {
            handlers.addHandler(createBpcContext(instance));
        }
    }

    private void addDispatcherContext(ContextHandlerCollection handlers) {

        HashMap<String, String> sessionStore = new HashMap<String, String>();

        // two dispatchers are created to make context separation easier
        TestRequestDispatcher bpDispatcher = new TestRequestDispatcher(sessionStore, serverInstances);
        bpDispatcher.setContextPath(SL + CONTEXT_DISPATCHER + SL + CONTEXT_BOUNCEPROXY);
        handlers.addHandler(bpDispatcher);

        TestRequestDispatcher bpcDispatcher = new TestRequestDispatcher(sessionStore, serverInstances);
        bpcDispatcher.setContextPath(SL + CONTEXT_DISPATCHER + SL + CONTEXT_CONTROLLER);
        handlers.addHandler(bpcDispatcher);
    }

    private WebAppContext createBpcContext(ClusterNode instance) {

        Properties bpcProps = new Properties();
        bpcProps.setProperty("joynr.bounceproxy.controller.bp_cache_name", BP_CACHE_NAME);
        bpcProps.setProperty("joynr.bounceproxy.controller.bp_cache_config_file", instance.getEhcacheConfigFile());

        return ServersUtil.createBounceproxyControllerWebApp("bounceproxy-controller-clustered",
                                                             instance.getContextPath(),
                                                             bpcProps);
    }

    private WebAppContext createBpContext(ClusterNode instance) {

        String bounceProxyUrl = serverBaseUrl + instance.getContextPath() + SL + CONTEXT_BOUNCEPROXY + SL;
        String bounceProxyDispatcherUrl = serverBaseUrl + CONTEXT_DISPATCHER + SL + CONTEXT_BOUNCEPROXY + SL;

        Properties bpProps = new Properties();
        bpProps.setProperty("joynr.bounceproxy.id", instance.getBounceProxyId());
        bpProps.setProperty("joynr.bounceproxy.controller.baseurl", bpcDispatcherUrl);
        bpProps.setProperty("joynr.bounceproxy.url4cc", bounceProxyDispatcherUrl);
        bpProps.setProperty("joynr.bounceproxy.url4bpc", bounceProxyUrl);
        bpProps.setProperty("jvmRoute", String.valueOf(instance.getInstanceId()));

        return ServersUtil.createControlledBounceproxyWebApp(instance.getContextPath(), bpProps);
    }

    private void waitForBounceProxyRegistrationAtAllControllers(long timeout_ms) throws InterruptedException {

        String[] bounceProxyIds = new String[serverInstances.size()];
        int i = 0;
        for (ClusterNode serverInstance : serverInstances.values()) {
            String bounceProxyId = serverInstance.getBounceProxyId();
            bounceProxyIds[i++] = bounceProxyId;
        }

        long overall_start_ms = System.currentTimeMillis();
        for (ClusterNode serverInstance : serverInstances.values()) {

            long bpc_start_ms = System.currentTimeMillis();
            long remaining_timeout_ms = timeout_ms - (bpc_start_ms - overall_start_ms);

            String bounceProxyControllerUrl = String.format("%s%s/%s/",
                                                            serverBaseUrl,
                                                            serverInstance.getContextPath(),
                                                            CONTEXT_CONTROLLER);
            if (!ServersUtil.waitForBounceProxyRegistration(remaining_timeout_ms,
                                                            1000l,
                                                            bounceProxyControllerUrl,
                                                            bounceProxyIds)) {
                Assert.fail("Bounce proxies did not register with in time");
            }
        }

    }

}
