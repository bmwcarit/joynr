package io.joynr.messaging.bounceproxy.runtime;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.bounceproxy.ControlledBounceProxyModule;
import io.joynr.messaging.bounceproxy.monitoring.MonitoringServiceClient;
import io.joynr.messaging.service.ChannelServiceRestAdapter;
import io.joynr.runtime.PropertyLoader;

import javax.servlet.ServletContextEvent;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.servlet.GuiceServletContextListener;
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 * Servlet configuration for controlled bounceproxy servlet.
 * 
 * @author christina.strobel
 * 
 */
public class ControlledBounceProxyServletConfig extends GuiceServletContextListener {

    private JerseyServletModule jerseyServletModule;
    private Injector injector;

    @Override
    public void contextInitialized(ServletContextEvent servletContextEvent) {

        // The jerseyServletModule injects the servicing classes using guice,
        // instead of letting jersey do it natively
        jerseyServletModule = new JerseyServletModule() {

            @Override
            protected void configureServlets() {

                bind(ChannelServiceRestAdapter.class);

                // Route all requests through GuiceContainer
                serve("/*").with(GuiceContainer.class);

                // Filter to only let requests pass if the bounce proxy has been
                // initialized correctly, e.g. if it has registered with the
                // bounce proxy controller.
                filter("/*").through(BounceProxyInitializedFilter.class);
            }

        };

        injector = Guice.createInjector(new PropertyLoadingModule(PropertyLoader.loadProperties("controlledBounceProxy.properties"),
                                                                  BounceProxySystemPropertyLoader.loadProperties()),
                                        jerseyServletModule,
                                        new ControlledBounceProxyModule());

        // Hook to send out message that bounce proxy has started and to start
        // the performance monitoring loop.
        MonitoringServiceClient monitoringServiceClient = injector.getInstance(MonitoringServiceClient.class);
        monitoringServiceClient.startStartupReporting();

        monitoringServiceClient.startPerformanceReport();

        super.contextInitialized(servletContextEvent);
    }

    @Override
    public void contextDestroyed(ServletContextEvent servletContextEvent) {

        // Hook to send out message that the bounce proxy will shutdown.
        MonitoringServiceClient monitoringServiceClient = injector.getInstance(MonitoringServiceClient.class);
        monitoringServiceClient.reportShutdown();

        super.contextDestroyed(servletContextEvent);
    }

    @Override
    protected Injector getInjector() {
        return injector;
    }
}