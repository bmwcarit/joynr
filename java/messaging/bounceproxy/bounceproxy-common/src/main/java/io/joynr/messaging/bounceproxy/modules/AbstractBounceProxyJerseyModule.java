package io.joynr.messaging.bounceproxy.modules;

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

import io.joynr.guice.servlet.AbstractJoynrServletModule;
import io.joynr.messaging.bounceproxy.filter.CharacterEncodingFilter;
import io.joynr.messaging.bounceproxy.filter.CorsFilter;
import io.joynr.messaging.service.ChannelServiceRestAdapter;
import io.joynr.messaging.service.MessagingServiceRestAdapter;

import org.atmosphere.guice.GuiceManagedAtmosphereServlet;

/**
 * Common Jersey module for bounce proxies to define servlet bindings. <br>
 * It binds servlets that have to be implemented by all bounce proxies. Any
 * servlets specific to a certain bounceproxy implementation have to be derived
 * by the subclass.<br>
 * It also defines to route all requests through
 * {@link GuiceManagedAtmosphereServlet}.
 * 
 * @author christina.strobel
 * 
 */
public abstract class AbstractBounceProxyJerseyModule extends AbstractJoynrServletModule {

    @Override
    protected void configureJoynrServlets() {
        bind(ChannelServiceRestAdapter.class);
        bind(MessagingServiceRestAdapter.class);
        bindServlets();

        filter("/*").through(CharacterEncodingFilter.class);
        filter("/*").through(CorsFilter.class);
        setupFilters();
    }

    @Override
    protected void bindJoynrServletClass() {
        serve("/*").with(GuiceManagedAtmosphereServlet.class, getAtmosphereModule().getParameters());
    }

    /**
     * Returns the atmosphere module.
     * 
     * @return the atmosphere module
     */
    protected abstract AtmosphereModule getAtmosphereModule();

    /**
     * Within this call, all filters specific to the bounceproxy implementation
     * have to be setup.
     */
    protected abstract void setupFilters();

    /**
     * Within this call, all servlets specific to the bounceproxy implementation
     * have to be setup.
     */
    protected abstract void bindServlets();

}
