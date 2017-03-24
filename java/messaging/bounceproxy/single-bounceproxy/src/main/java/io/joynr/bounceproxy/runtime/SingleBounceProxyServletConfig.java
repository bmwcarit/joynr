package io.joynr.bounceproxy.runtime;

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

import io.joynr.bounceproxy.SingleBounceProxyModule;
import io.joynr.bounceproxy.service.AttachmentReceiverService;
import io.joynr.bounceproxy.service.AttachmentSenderService;
import io.joynr.bounceproxy.service.MessagingWithoutContentTypeService;
import io.joynr.bounceproxy.service.TimeService;
import io.joynr.guice.PropertyLoadingModule;
import io.joynr.guice.servlet.AbstractGuiceServletConfig;
import io.joynr.guice.servlet.AbstractJoynrServletModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.bounceproxy.modules.AbstractBounceProxyJerseyModule;
import io.joynr.messaging.bounceproxy.modules.AtmosphereModule;
import io.joynr.messaging.bounceproxy.modules.DefaultBounceProxyModule;
import io.joynr.runtime.PropertyLoader;

import java.util.LinkedList;
import java.util.List;
import java.util.Properties;

import com.google.inject.Module;
import com.google.inject.util.Modules;

/**
 * Servlet configuration for single bounceproxy servlet.
 * 
 * @author christina.strobel
 * 
 */
public class SingleBounceProxyServletConfig extends AbstractGuiceServletConfig {

    private static final String BOUNCEPROXY_PROPERTIES = "bounceProxy.properties";

    private final List<Module> modules;

    private final AtmosphereModule atmosphereModule;

    public SingleBounceProxyServletConfig() {

        Properties bounceProxySystemProperties = new Properties();
        String hostPath = System.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH);
        if (hostPath != null) {
            bounceProxySystemProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, hostPath);
        }

        atmosphereModule = new AtmosphereModule();

        modules = new LinkedList<Module>();
        modules.add(new PropertyLoadingModule(PropertyLoader.loadProperties(BOUNCEPROXY_PROPERTIES),
                                              bounceProxySystemProperties));
        modules.add(Modules.override(new DefaultBounceProxyModule()).with(new SingleBounceProxyModule()));
        modules.add(atmosphereModule);
    }

    @Override
    protected AbstractJoynrServletModule getJoynrServletModule() {
        return new AbstractBounceProxyJerseyModule() {

            @Override
            protected void bindServlets() {
                bind(TimeService.class);
                bind(AttachmentReceiverService.class);
                bind(AttachmentSenderService.class);
                bind(MessagingWithoutContentTypeService.class);
            }

            @Override
            protected void setupFilters() {
            }

            protected AtmosphereModule getAtmosphereModule() {
                return atmosphereModule;
            }

        };
    }

    @Override
    protected List<Module> getJoynrModules() {
        return modules;
    }

}
