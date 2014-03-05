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

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.guice.servlet.AbstractGuiceServletConfig;
import io.joynr.guice.servlet.AbstractJoynrServletModule;
import io.joynr.messaging.bounceproxy.controller.BounceProxyControllerModule;
import io.joynr.messaging.service.ChannelServiceRestAdapter;
import io.joynr.messaging.service.MonitoringServiceRestAdapter;
import io.joynr.runtime.PropertyLoader;

import java.util.LinkedList;
import java.util.List;

import com.google.inject.Module;

/**
 * Servlet configuration for bounceproxy controller servlet.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyControllerServletConfig extends AbstractGuiceServletConfig {

    private final List<Module> modules;

    public BounceProxyControllerServletConfig() {
        modules = new LinkedList<Module>();
        modules.add(new PropertyLoadingModule(PropertyLoader.loadProperties("bounceProxyController.properties")));
        modules.add(new BounceProxyControllerModule());
    }

    @Override
    protected AbstractJoynrServletModule getJoynrServletModule() {
        return new AbstractJoynrServletModule() {

            @Override
            protected void configureJoynrServlets() {
                bind(ChannelServiceRestAdapter.class);
                bind(MonitoringServiceRestAdapter.class);
            }
        };
    }

    @Override
    protected List<Module> getJoynrModules() {
        return modules;
    }
}
