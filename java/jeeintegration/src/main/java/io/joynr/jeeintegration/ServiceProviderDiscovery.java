/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import static io.joynr.util.AnnotationUtil.getAnnotation;
import static java.lang.String.format;

import java.util.HashSet;
import java.util.Set;

import jakarta.enterprise.inject.Any;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.enterprise.util.AnnotationLiteral;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.ProvidedBy;
import io.joynr.jeeintegration.api.ServiceProvider;

/**
 * This class is responsible for finding all beans registered in the {@link BeanManager} which are annotated with
 * {@link ServiceProvider}.
 */
public class ServiceProviderDiscovery {

    private static final Logger logger = LoggerFactory.getLogger(ServiceProviderDiscovery.class);

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
            if (serviceProvider != null) {
                ProvidedBy providedBy = getProvidedByAnnotation(serviceProvider.serviceInterface());
                verifyProvidedBy(providedBy, serviceProvider.serviceInterface(), bean);
                result.add(bean);
                if (logger.isTraceEnabled()) {
                    logger.trace(format("Bean %s is a service provider. Adding to result.", bean));
                }
            } else if (logger.isTraceEnabled()) {
                logger.trace(format("Ignoring bean: %s", bean));
            }
        }
        if (logger.isDebugEnabled()) {
            logger.debug(format("Found the following service provider beans:%n%s", result));
        }
        return result;
    }

    private void verifyProvidedBy(ProvidedBy providedBy, Class<?> serviceInterface, Bean<?> bean) {
        if (providedBy == null) {
            throw new IllegalArgumentException(format("The bean %s is annotated with ServiceProvider, but the"
                    + " specified service interface %s is not annotated with @ProvidedBy."
                    + "%nMake sure that you implement a *Sync interface and"
                    + " that you specify that same *Sync interface as the serviceInterface."
                    + "%nNote: Are you using an outdated version of the generator?"
                    + "%nIn older versions of the generator, you have to set the now deprecated 'jee' "
                    + "parameter to 'true' in order to correctly generate the @ProvidedBy annotation on "
                    + "the Sync interfaces. This annoation is now generated automatically.", bean, serviceInterface));
        }
    }

    /**
     * Use this method to get the joynr provider interface for the given business interface.
     * The business interface is mapped to its joynr provider, by expecting a {@link io.joynr.ProvidedBy} annotation
     * is attached in the class hierarchy of the business interface. With this annotation, the provider is found by
     * the ServiceProviderDiscovery. 
     *
     * @param businessInterface
     *            the business interface for which we want to find the joynr provider interface.
     *
     * @return the provider interface, if found, or the interface passed in if not.
     */
    public Class<?> getProviderInterfaceFor(Class<?> businessInterface) {
        if (logger.isTraceEnabled()) {
            logger.trace(format("Looking for provider interface for business interface %s", businessInterface));
        }
        assert businessInterface != null : "businessInterface must not be null";
        ProvidedBy providedBy = getProvidedByAnnotation(businessInterface);

        if (providedBy == null) {
            throw new IllegalArgumentException(format("Unable to find suitable joynr provider for interface %s",
                                                      businessInterface));
        } else {
            Class<?> result = providedBy.value();
            if (logger.isTraceEnabled()) {
                logger.trace(format("Returning: %s", result));
            }
            return result;
        }
    }

    private ProvidedBy getProvidedByAnnotation(Class<?> businessInterface) {
        return getAnnotation(businessInterface, ProvidedBy.class);
    }

}
