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
import io.joynr.guice.servlet.AbstractGuiceServletConfig;
import io.joynr.guice.servlet.AbstractJoynrServletModule;
import io.joynr.messaging.bounceproxy.BounceProxyBroadcaster;
import io.joynr.messaging.bounceproxy.ControlledBounceProxyModule;
import io.joynr.messaging.bounceproxy.monitoring.MonitoringServiceClient;
import io.joynr.messaging.service.ChannelServiceRestAdapter;
import io.joynr.runtime.PropertyLoader;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.servlet.ServletContextEvent;

import org.atmosphere.cache.UUIDBroadcasterCache;
import org.atmosphere.guice.GuiceManagedAtmosphereServlet;

import com.google.inject.Module;
import com.google.inject.TypeLiteral;
import com.google.inject.name.Names;

/**
 * Servlet configuration for controlled bounceproxy servlet.
 * 
 * @author christina.strobel
 * 
 */
public class ControlledBounceProxyServletConfig extends AbstractGuiceServletConfig {

    private final List<Module> modules;
    Map<String, String> params = new HashMap<String, String>();

    public ControlledBounceProxyServletConfig() {
        modules = new LinkedList<Module>();
        modules.add(new PropertyLoadingModule(PropertyLoader.loadProperties("controlledBounceProxy.properties"),
                                              BounceProxySystemPropertyLoader.loadProperties()));
        modules.add(new ControlledBounceProxyModule());
    }

    @Override
    public void contextInitialized(ServletContextEvent servletContextEvent) {

        super.contextInitialized(servletContextEvent);

        // Hook to send out message that bounce proxy has started and to start
        // the performance monitoring loop.
        MonitoringServiceClient monitoringServiceClient = getInjector().getInstance(MonitoringServiceClient.class);
        monitoringServiceClient.startStartupReporting();

        monitoringServiceClient.startPerformanceReport();
    }

    @Override
    public void contextDestroyed(ServletContextEvent servletContextEvent) {

        // Hook to send out message that the bounce proxy will shutdown.
        MonitoringServiceClient monitoringServiceClient = getInjector().getInstance(MonitoringServiceClient.class);
        monitoringServiceClient.reportShutdown();

        super.contextDestroyed(servletContextEvent);
    }

    @Override
    protected AbstractJoynrServletModule getJoynrServletModule() {
        return new AbstractJoynrServletModule() {

            @Override
            protected void configureJoynrServlets() {
                bind(ChannelServiceRestAdapter.class);

                // Filter to only let requests pass if the bounce proxy has been
                // initialized correctly, e.g. if it has registered with the
                // bounce proxy controller.
                filter("/*").through(BounceProxyInitializedFilter.class);

                // TODO put configuration somewhere else
                // This will be done with refactoring of the bounceproxy,
                // when bounceproxy is also configured with Guice.
                params.put("suspend.seconds", "20");
                params.put("com.sun.jersey.config.property.packages", "io.joynr.messaging.bounceproxy");
                params.put("org.atmosphere.cpr.broadcasterClass", BounceProxyBroadcaster.class.getName());
                params.put("org.atmosphere.cpr.broadcasterCacheClass", UUIDBroadcasterCache.class.getName());
                params.put("org.atmosphere.useBlocking", "false");
                params.put("org.atmosphere.cpr.broadcasterLifeCyclePolicy", "NEVER");
                params.put("org.atmosphere.cpr.broadcaster.shareableThreadPool", "true");
                params.put("com.sun.jersey.config.feature.DisableWADL", "true");
                params.put("org.atmosphere.cpr.BroadcasterCache.strategy", "beforeFilter");

                bind(new TypeLiteral<Map<String, String>>() {
                }).annotatedWith(Names.named("org.atmosphere.guice.AtmosphereGuiceServlet.properties"))
                  .toInstance(params);
            }

            @Override
            protected void bindJoynrServletClass() {
                serve("/*").with(GuiceManagedAtmosphereServlet.class, params);
            }

        };
    }

    @Override
    protected List<Module> getJoynrModules() {
        return modules;
    }
}