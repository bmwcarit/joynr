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

import static java.lang.String.format;

import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.Set;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import javax.ejb.DependsOn;
import javax.ejb.Singleton;
import javax.ejb.Startup;
import javax.enterprise.inject.Any;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.util.AnnotationLiteral;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;

import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ProviderRegistrationSettingsFactory;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.provider.JoynrProvider;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownNotifier;
import joynr.types.ProviderQos;

/**
 * Singleton EJB which will create and keep the Joynr runtime and also shut it down when the app server shuts down as
 * well as scan the system for all CDI beans present which are annotated with {@link ServiceProvider} and register those
 * as joynr providers with the joynr runtime.
 */
@Singleton
@Startup
@DependsOn("JeeJoynrServiceLocator")
public class JoynrIntegrationBean {

    private static final Logger logger = LoggerFactory.getLogger(JoynrIntegrationBean.class);

    private BeanManager beanManager;

    private JoynrRuntimeFactory joynrRuntimeFactory;

    private ServiceProviderDiscovery serviceProviderDiscovery;

    private CallbackHandlerDiscovery callbackHandlerDiscovery;

    private Set<Object> registeredProviders = new HashSet<>();

    private JoynrRuntime joynrRuntime;

    public JoynrIntegrationBean() {
    }

    @Inject
    public JoynrIntegrationBean(BeanManager beanManager,
                                JoynrRuntimeFactory joynrRuntimeFactory,
                                ServiceProviderDiscovery serviceProviderDiscovery,
                                CallbackHandlerDiscovery callbackHandlerDiscovery) {
        this.beanManager = beanManager;
        this.joynrRuntimeFactory = joynrRuntimeFactory;
        this.serviceProviderDiscovery = serviceProviderDiscovery;
        this.callbackHandlerDiscovery = callbackHandlerDiscovery;
    }

    @PostConstruct
    public void initialise() {
        logger.debug("Initializing joynr integration bean");
        Set<Bean<?>> serviceProviderBeans = serviceProviderDiscovery.findServiceProviderBeans();
        joynrRuntime = joynrRuntimeFactory.create(getServiceProviderInterfaceClasses(serviceProviderBeans));
        registerProviders(serviceProviderBeans, joynrRuntime);
        registerCallbackHandlers(joynrRuntime);
    }

    private void registerCallbackHandlers(JoynrRuntime joynrRuntime) {
        callbackHandlerDiscovery.forEach(callbackBean -> {
            joynrRuntime.registerStatelessAsyncCallback(callbackBean);
        });
    }

    private void registerProviders(Set<Bean<?>> serviceProviderBeans, JoynrRuntime runtime) {
        Set<ProviderRegistrationSettingsFactory> providerSettingsFactories = getProviderRegistrationSettingsFactories();

        for (Bean<?> bean : serviceProviderBeans) {
            Class<?> beanClass = bean.getBeanClass();
            Class<?> serviceInterface = beanClass.getAnnotation(ServiceProvider.class).serviceInterface();
            Class<?> providerInterface = serviceProviderDiscovery.getProviderInterfaceFor(serviceInterface);
            if (logger.isDebugEnabled()) {
                logger.debug(format("Registering in joynr runtime the bean %s as provider %s for service %s.",
                                    bean,
                                    providerInterface,
                                    serviceInterface));
            }
            JoynrProvider provider = (JoynrProvider) Proxy.newProxyInstance(beanClass.getClassLoader(),
                                                                            new Class<?>[]{ providerInterface,
                                                                                    JoynrProvider.class },
                                                                            new ProviderWrapper(bean,
                                                                                                beanManager,
                                                                                                joynrRuntimeFactory.getInjector()));

            // try to find customized settings for the registration
            ProviderQos providerQos = null;
            String[] gbids = null;

            for (ProviderRegistrationSettingsFactory factory : providerSettingsFactories) {
                if (factory.providesFor(serviceInterface)) {
                    providerQos = factory.createProviderQos();
                    gbids = factory.createGbids();
                    break;
                }
            }

            if (providerQos == null) {
                providerQos = new ProviderQos();
            }

            if (gbids == null) {
                // empty array for default registration (i.e. in default backend)
                gbids = new String[0];
            }

            runtime.getProviderRegistrar(getDomainForProvider(beanClass), provider)
                   .withProviderQos(providerQos)
                   .withGbids(gbids)
                   .register();

            registeredProviders.add(provider);
        }
    }

    private String getDomainForProvider(Class<?> beanClass) {
        String domain;
        ProviderDomain providerDomain = beanClass.getAnnotation(ProviderDomain.class);
        if (providerDomain != null) {
            domain = providerDomain.value();
        } else {
            domain = joynrRuntimeFactory.getLocalDomain();
        }
        return domain;
    }

    /**
     * A util method to find factories which provide some customized settings needed for
     * registration of joynr providers, i.e. implementations of the interface
     * {@link ProviderRegistrationSettingsFactory}.
     *
     * @return set of factories implementing the interface
     */
    @SuppressWarnings({ "rawtypes", "unchecked", "serial" })
    private Set<ProviderRegistrationSettingsFactory> getProviderRegistrationSettingsFactories() {
        Set<Bean<?>> providerSettingsFactoryBeans = beanManager.getBeans(ProviderRegistrationSettingsFactory.class,
                                                                         new AnnotationLiteral<Any>() {
                                                                         });
        Set<ProviderRegistrationSettingsFactory> providerSettingsFactories = new HashSet<>();
        for (Bean providerSettingsFactoryBean : providerSettingsFactoryBeans) {
            ProviderRegistrationSettingsFactory factory = (ProviderRegistrationSettingsFactory) providerSettingsFactoryBean.create(beanManager.createCreationalContext(providerSettingsFactoryBean));
            providerSettingsFactories.add(factory);
        }
        return providerSettingsFactories;
    }

    @PreDestroy
    public void destroy() {
        if (!(joynrRuntimeFactory.getProperties()
                                 .getProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "false")
                                 .equals("true"))) {
            logger.info("Unregistering provider ", joynrRuntimeFactory.getLocalDomain());
            for (Object provider : registeredProviders) {
                try {
                    joynrRuntime.unregisterProvider(joynrRuntimeFactory.getLocalDomain(), provider);
                } catch (Exception e) {
                    logger.error("Error unregistering provider", e);
                }
            }
        }
        ShutdownNotifier shutdownNotifier = getJoynrInjector().getInstance(ShutdownNotifier.class);
        shutdownNotifier.shutdown();
    }

    /**
     * Can be used to get access to the Guice Injector used in the joynr application / runtime.
     *
     * @return the Guice Injector used by the joynr application if the application has already been
     *         {@link #initialise() started}, or <code>null</code> if not.
     */
    public Injector getJoynrInjector() {
        return joynrRuntimeFactory.getInjector();
    }

    private Set<Class<?>> getServiceProviderInterfaceClasses(Set<Bean<?>> serviceProviderBeans) {
        Set<Class<?>> result = new HashSet<>();
        for (Bean<?> bean : serviceProviderBeans) {
            result.add(bean.getBeanClass().getAnnotation(ServiceProvider.class).serviceInterface());
        }
        return result;
    }

    public JoynrRuntime getRuntime() {
        return joynrRuntime;
    }

}
