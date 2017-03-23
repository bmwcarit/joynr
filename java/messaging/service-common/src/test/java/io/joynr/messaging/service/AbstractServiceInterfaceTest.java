package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::service-common
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

import javax.servlet.ServletContextEvent;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.webapp.WebAppContext;
import org.junit.After;
import org.junit.Before;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.servlet.GuiceFilter;
import com.google.inject.servlet.GuiceServletContextListener;
import com.google.inject.servlet.ServletModule;

/**
 * Abstract test class that starts an embedded jetty server to test servlets on.
 * 
 * @author christina.strobel
 * 
 */
public abstract class AbstractServiceInterfaceTest {

    private String serverUrl;

    private Server jettyServer;

    @Before
    public void setUp() throws Exception {

        // starts the server with a random port
        jettyServer = new Server(0);

        WebAppContext bpCtrlWebapp = new WebAppContext();
        bpCtrlWebapp.setResourceBase("./src/main/java");
        bpCtrlWebapp.setParentLoaderPriority(true);

        bpCtrlWebapp.addFilter(GuiceFilter.class, "/*", null);
        bpCtrlWebapp.addEventListener(new GuiceServletContextListener() {

            private Injector injector;

            @Override
            public void contextInitialized(ServletContextEvent servletContextEvent) {
                injector = Guice.createInjector(getServletTestModule());
                super.contextInitialized(servletContextEvent);
            }

            @Override
            protected Injector getInjector() {
                return injector;
            }
        });

        jettyServer.setHandler(bpCtrlWebapp);

        jettyServer.start();

        int port = ((ServerConnector) jettyServer.getConnectors()[0]).getLocalPort();
        serverUrl = String.format("http://localhost:%d", port);
    }

    @After
    public void tearDown() throws Exception {
        jettyServer.stop();
    }

    protected abstract ServletModule getServletTestModule();

    public String getServerUrlWithoutPath() {
        return serverUrl;
    }
}
