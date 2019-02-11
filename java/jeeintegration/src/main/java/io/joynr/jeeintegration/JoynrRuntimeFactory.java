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
package io.joynr.jeeintegration;

import java.util.Set;

import com.google.inject.Injector;

import io.joynr.runtime.JoynrRuntime;

/**
 * Used by the JEE integration bean in order to obtain the joynr application to use. This can principally be a new
 * instance every time, but a singleton probably makes more sense.
 *
 * @see JoynrIntegrationBean
 */
public interface JoynrRuntimeFactory {

    /**
     * Call this method in order to create or obtain an instance of the joynr application this factory provides. The
     * returned joynr application should be initialised, but not yet running.
     * 
     * @param providerInterfaceClasses
     *            the set of provider interface classes which will be provisioned for the joynr runtime. These are
     *            required in order to set up the access control entries during startup.
     *
     * @return the instance of the joynr application this factory provides.
     */
    JoynrRuntime create(Set<Class<?>> providerInterfaceClasses);

    /**
     * Provides the name of the local domain with which all providers will be registered.
     *
     * @return the local domain name with which providers are registered.
     */
    String getLocalDomain();

    /**
     * Provides the guice injector to be used by the joynr runtime factory
     * @return the guice intector to be used by the joynr runtime factory
     */
    Injector getInjector();

}
