package io.joynr.messaging.bounceproxy.controller.runtime;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.bounceproxy.controller.BounceProxyControllerModule;
import io.joynr.messaging.service.ChannelServiceRestAdapter;
import io.joynr.messaging.service.MonitoringServiceRestAdapter;

import javax.servlet.ServletContextEvent;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.servlet.GuiceServletContextListener;
import com.sun.jersey.guice.JerseyServletModule;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 * Servlet configuration for bounceproxy controller servlet.
 * 
 * @author christina.strobel
 *
 */
public class BounceProxyControllerServletConfig extends GuiceServletContextListener {

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
                bind(MonitoringServiceRestAdapter.class);

                // Route all requests through GuiceContainer
                serve("/*").with(GuiceContainer.class);
            }

        };

        injector = Guice.createInjector(jerseyServletModule, new BounceProxyControllerModule());

        super.contextInitialized(servletContextEvent);
    }

    @Override
    protected Injector getInjector() {
        return injector;
    }
}
