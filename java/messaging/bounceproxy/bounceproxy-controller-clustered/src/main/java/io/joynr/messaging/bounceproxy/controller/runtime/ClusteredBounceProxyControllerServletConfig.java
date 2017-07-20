package io.joynr.messaging.bounceproxy.controller.runtime;

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

import java.util.LinkedList;
import java.util.List;

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.bounceproxy.controller.directory.ehcache.EhcacheModule;
import io.joynr.runtime.PropertyLoader;

import com.google.inject.AbstractModule;

/**
 * Servlet configuration for a clustered bounceproxy controller servlet, i.e.
 * which can share necessary data between several servlet instances in one
 * cluster.
 * 
 * @author christina.strobel
 * 
 */
public class ClusteredBounceProxyControllerServletConfig extends AbstractBounceProxyControllerServletConfig {

    @Override
    protected List<AbstractModule> getPersistenceModules() {
        LinkedList<AbstractModule> persistenceModules = new LinkedList<AbstractModule>();
        persistenceModules.add(new PropertyLoadingModule(PropertyLoader.loadProperties("ehcache-persistence.properties"),
                                                         getJoynrSystemProperties()));
        persistenceModules.add(new EhcacheModule());
        return persistenceModules;
    }

}
