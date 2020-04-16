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
package io.joynr.runtime;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.Module;
import com.google.inject.name.Names;

import io.joynr.guice.IApplication;
import io.joynr.guice.InjectorFactory;
import io.joynr.messaging.MessagingPropertyKeys;

/**
 * This class is used as abstract joynr injector factory. Client code uses sub classes of this to instantiate joynr
 * applications via the createApplication method and to get injector objects for their own object instantiations.
 * The injection binding is based on the modules provided via the constructor of this class.
 */
public abstract class AbstractJoynrInjectorFactory extends InjectorFactory<JoynrApplicationModule, JoynrApplication> {

    private static final Logger logger = LoggerFactory.getLogger(AbstractJoynrInjectorFactory.class);

    /**
     * @param modules - arbitrary set of modules used to bind injection elements while creating injectors or joynr
     *                  applications
     */
    public AbstractJoynrInjectorFactory(Module... modules) {
        super(modules);
    }

    @Override
    public JoynrApplication createApplication(JoynrApplicationModule applicationModule,
                                              Module... applicationSpecificModules) {
        Module[] modules = new Module[applicationSpecificModules != null ? applicationSpecificModules.length + 1 : 1];
        modules[0] = applicationModule;
        if (applicationSpecificModules != null) {
            for (int i = 0; i < applicationSpecificModules.length; i++) {
                modules[i + 1] = applicationSpecificModules[i];
            }
        }

        Injector fullInjector = getInjector().createChildInjector(modules);

        String channelId = fullInjector.getInstance(Key.get(String.class,
                                                            Names.named(MessagingPropertyKeys.CHANNELID)));
        logger.info("Application using channelId {}.", channelId);

        IApplication result = fullInjector.getInstance(IApplication.class);
        if (result instanceof JoynrApplication) {
            return (JoynrApplication) result;
        }
        throw new AssertionError("The injector factory " + this.getClass()
                + " has been configured wrong: io.joynr.guice.IApplication has not been bind to a suptype of "
                + JoynrApplication.class.getSimpleName());
    }

    /**
     * This function is used to update the injector factory with new bindings
     * @param customJoynProperties - properties to be used as binding for the injectors
     * @param modules - modules to be uses as binding for the injectors
     */
    public abstract void updateInjectorModule(Properties customJoynProperties, Module... modules);
}
