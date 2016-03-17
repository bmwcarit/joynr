/**
 *
 */
package io.joynr.jeeintegration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static java.lang.String.format;

import java.util.HashSet;
import java.util.Set;

import javax.enterprise.inject.Any;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.util.AnnotationLiteral;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.provider.JoynrProvider;

/**
 * This class is reponsible for finding all beans registered in the {@link BeanManager} which are annotated with
 * {@link ServiceProvider}.
 *
 * @author Clive Jevons <clive@jevons-it.net> commissioned by MaibornWolff
 */
public class ServiceProviderDiscovery {

    private static final Logger LOG = LoggerFactory.getLogger(ServiceProviderDiscovery.class);

    private final BeanManager beanManager;

    @Inject
    public ServiceProviderDiscovery(BeanManager beanManager) {
        this.beanManager = beanManager;
    }

    @SuppressWarnings("serial")
    public Set<Bean<?>> findServiceProviderBeans() {
        Set<Bean<?>> result = new HashSet<>();
        for (Bean<?> bean : beanManager.getBeans(Object.class, new AnnotationLiteral<Any>() {
        })) {
            ServiceProvider serviceProvider = bean.getBeanClass().getAnnotation(ServiceProvider.class);
            if (serviceProvider != null
                    && JoynrProvider.class.isAssignableFrom(getProviderInterfaceFor(serviceProvider.serviceInterface()))) {
                if (LOG.isTraceEnabled()) {
                    LOG.trace(format("Bean %s is a service provider. Adding to result.", bean));
                }
                result.add(bean);
            } else if (LOG.isTraceEnabled()) {
                LOG.trace(format("Ignoring bean: %s", bean));
            }
        }
        if (LOG.isDebugEnabled()) {
            LOG.debug(format("Found the following service provider beans:%n%s", result));
        }
        return result;
    }

    /**
     * Use this method to get the joynr provider interface for the given business interface. We do this be convention.
     * We substitute the business interface ending 'BCI' for the provider interface ending 'Provider' and try to load
     * that class.
     *
     * @param businessInterface
     *            the business interface for which we want to find the joynr provider interface.
     *
     * @return the provider interface, if found, or the interface passed in if not.
     */
    public Class<?> getProviderInterfaceFor(Class<?> businessInterface) {
        if (LOG.isTraceEnabled()) {
            LOG.trace(format("Looking for provider interface for business interface %s", businessInterface));
        }
        assert businessInterface != null : "businessInterface must not be null";
        String providerInterfaceName = businessInterface.getName().replaceFirst("BCI$", "Provider");
        Class<?> result;
        try {
            result = Class.forName(providerInterfaceName);
        } catch (ClassNotFoundException e) {
            LOG.warn(format("Could find class named %s - defaulting to interface which was passed in.",
                            providerInterfaceName));
            result = businessInterface;
        }
        if (LOG.isTraceEnabled()) {
            LOG.trace(format("Returning: %s", result));
        }
        return result;
    }

}
