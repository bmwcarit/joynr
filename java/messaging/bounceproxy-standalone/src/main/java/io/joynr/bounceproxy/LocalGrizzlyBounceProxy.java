package io.joynr.bounceproxy;

/*
 * #%L
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

import io.joynr.bounceproxy.filter.CorsFilter;
import io.joynr.servlet.ServletUtil;

import java.io.IOException;

import org.atmosphere.container.GrizzlyCometSupport;

import com.sun.grizzly.comet.CometAsyncFilter;
import com.sun.grizzly.http.embed.GrizzlyWebServer;
import com.sun.grizzly.http.servlet.ServletAdapter;

public class LocalGrizzlyBounceProxy {

    private GrizzlyWebServer server;
    private int port;
    public static final String TOP_LEVEL_MAPPING = "";

    protected String root = "/bounceproxy";
    private static final int SUSPEND_TIME_SECS = 10000;

    public LocalGrizzlyBounceProxy() {

    }

    public LocalGrizzlyBounceProxy(String root) {
        this.root = root;
    }

    public int start() throws IOException {
        int freePort = ServletUtil.findFreePort();
        start(freePort);
        return freePort;
    }

    public void start(int myPort) throws IOException {

        this.port = myPort;
        server = new GrizzlyWebServer(myPort);
        ServletAdapter sa = new ServletAdapter();
        sa.setContextPath(root);
        sa.addFilter(new CorsFilter(), "cross-origin", null);
        server.addAsyncFilter(new CometAsyncFilter());
        BounceProxyServletContainer servletInstance = new BounceProxyServletContainer();
        servletInstance.framework().setAsyncSupport(new GrizzlyCometSupport(servletInstance.framework()
                                                                                           .getAtmosphereConfig()));

        // params for com.sun.jersey.config.property.packages. NOTE: multiple entries must be separated by ;
        StringBuilder packages = new StringBuilder();
        packages.append(this.getClass().getPackage().getName());
        // this causes Jersey to search for Root resource and provider classes ie. ChannelService and
        // MessagingContextResolver
        sa.addInitParameter("com.sun.jersey.config.property.packages", packages.toString());
        sa.addInitParameter("org.atmosphere.useNative", "false");
        sa.addInitParameter("org.atmosphere.useBlocking", "true");
        sa.addInitParameter("org.atmosphere.cpr.broadcaster.maxProcessingThreads", "500");
        sa.addInitParameter("org.atmosphere.cpr.AtmosphereInterceptor.disableDefaults", "true");
        sa.addInitParameter("org.atmosphere.cpr.broadcasterLifeCyclePolicy", "NEVER");
        sa.addInitParameter("org.atmosphere.cpr.broadcaster.shareableThreadPool", "true");
        sa.addInitParameter("org.atmosphere.cpr.broadcasterClass", "io.joynr.bounceproxy.BounceProxyBroadcaster");
        sa.addInitParameter("org.atmosphere.cpr.broadcasterCacheClass", "org.atmosphere.cache.UUIDBroadcasterCache");
        sa.addInitParameter("org.atmosphere.cpr.BroadcasterCache.strategy", "beforeFilter");

        sa.addInitParameter("suspend.seconds", String.valueOf(SUSPEND_TIME_SECS));

        sa.setServletInstance(servletInstance);
        server.addGrizzlyAdapter(sa, new String[]{ root + "/*" });
        server.start();
    }

    public void stop() {
        server.stop();

    }

    public int getPort() {
        return port;
    }

    public static void main(String[] args) {
        int port;
        String portProperty = System.getProperty("port");
        String contextProperty = System.getProperty("context");
        LocalGrizzlyBounceProxy localGrizzlyBounceProxy;

        // Set web application context (ie /bounceproxy) , or * if not given
        if (contextProperty != null) {
            localGrizzlyBounceProxy = new LocalGrizzlyBounceProxy(contextProperty);
        } else {
            localGrizzlyBounceProxy = new LocalGrizzlyBounceProxy();
        }

        try {
            if (portProperty != null) {

                port = Integer.parseInt(portProperty);
                localGrizzlyBounceProxy.start(port);
            } else {
                port = localGrizzlyBounceProxy.start();
            }
            System.out.println("Bounceproxy started on port: " + port);
        } catch (NumberFormatException e) {
            System.err.println("port not set");
        } catch (IOException e) {
            System.err.println("Bounceproxy not started:");
            System.err.println(e);

        }
    }

}
