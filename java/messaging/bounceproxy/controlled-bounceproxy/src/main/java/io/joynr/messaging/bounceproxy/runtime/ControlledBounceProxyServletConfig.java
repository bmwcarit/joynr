package io.joynr.messaging.bounceproxy.runtime;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.guice.servlet.AbstractGuiceServletConfig;
import io.joynr.guice.servlet.AbstractJoynrServletModule;
import io.joynr.messaging.bounceproxy.ControlledBounceProxyModule;
import io.joynr.messaging.bounceproxy.filter.SessionFilter;
import io.joynr.messaging.bounceproxy.modules.AbstractBounceProxyJerseyModule;
import io.joynr.messaging.bounceproxy.modules.AtmosphereModule;
import io.joynr.messaging.bounceproxy.modules.DefaultBounceProxyModule;
import io.joynr.messaging.bounceproxy.monitoring.MonitoringServiceClient;
import io.joynr.runtime.PropertyLoader;

import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import javax.servlet.ServletContextEvent;
import javax.servlet.SessionTrackingMode;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

/**
 * Servlet configuration for controlled bounceproxy servlet.
 * 
 * @author christina.strobel
 * 
 */
public class ControlledBounceProxyServletConfig extends AbstractGuiceServletConfig {

    private static final Logger logger = LoggerFactory.getLogger(ControlledBounceProxyServletConfig.class);

    private final List<Module> modules;

    private final AtmosphereModule atmosphereModule;

    public ControlledBounceProxyServletConfig() {

        atmosphereModule = new AtmosphereModule();

        modules = new LinkedList<Module>();
        modules.add(new PropertyLoadingModule(PropertyLoader.loadProperties("controlledBounceProxy.properties"),
                                              BounceProxySystemPropertyLoader.loadProperties(),
                                              PropertyLoader.loadProperties("session.properties")));
        modules.add(Modules.override(new DefaultBounceProxyModule()).with(new ControlledBounceProxyModule()));
        modules.add(atmosphereModule);
    }

    @Override
    public void contextInitialized(ServletContextEvent servletContextEvent) {

        super.contextInitialized(servletContextEvent);

        logSessionHandlingConfig(servletContextEvent);

        // Hook to send out message that bounce proxy has started and to start
        // the performance monitoring loop.
        MonitoringServiceClient monitoringServiceClient = getInjector().getInstance(MonitoringServiceClient.class);
        monitoringServiceClient.startStartupReporting();

        monitoringServiceClient.startPerformanceReport();
    }

    private void logSessionHandlingConfig(ServletContextEvent servletContextEvent) {

        Set<SessionTrackingMode> supportedTrackingModes = servletContextEvent.getServletContext()
                                                                             .getDefaultSessionTrackingModes();
        for (SessionTrackingMode trackingMode : supportedTrackingModes) {
            logger.info("Supported session tracking mode enabled: {}", trackingMode);
        }

        Set<SessionTrackingMode> effectiveTrackingModes = servletContextEvent.getServletContext()
                                                                             .getEffectiveSessionTrackingModes();
        for (SessionTrackingMode trackingMode : effectiveTrackingModes) {
            logger.info("Effective session tracking mode enabled: {}", trackingMode);
        }
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
        return new AbstractBounceProxyJerseyModule() {

            @Override
            protected void setupFilters() {
                // Filter to only let requests pass if the bounce proxy has been
                // initialized correctly, e.g. if it has registered with the
                // bounce proxy controller.
                filter("/*").through(BounceProxyInitializedFilter.class);
                filter("/*").through(SessionFilter.class);
            }

            @Override
            protected AtmosphereModule getAtmosphereModule() {
                return atmosphereModule;
            }

            @Override
            protected void bindServlets() {
            }
        };
    }

    @Override
    protected List<Module> getJoynrModules() {
        return modules;
    }
}