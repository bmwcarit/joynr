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

import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Optional;
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
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys;
import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ProviderRegistrationSettingsFactory;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.messaging.JeeSharedSubscriptionsMqttMessagingSkeleton;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ProviderRegistrar;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import joynr.system.RoutingTypes.MqttAddress;
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
        boolean sharedSubscriptionsEnabled = (getJoynrInjector().getInstance(Key.get(Boolean.class,
                                                                                     Names.named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS))));
        if (sharedSubscriptionsEnabled) {
            subscribeToSharedSubscriptionsTopic();
        }
    }

    private void subscribeToSharedSubscriptionsTopic() {
        String[] gbids = getJoynrInjector().getInstance(Key.get(String[].class,
                                                                Names.named(MessagingPropertyKeys.GBID_ARRAY)));
        MessagingSkeletonFactory factory = getJoynrInjector().getInstance(MessagingSkeletonFactory.class);
        Arrays.stream(gbids).forEach(gbid -> {
            Optional<IMessagingSkeleton> skeleton = factory.getSkeleton(new MqttAddress(gbid, ""));
            if (!skeleton.isPresent()) {
                throw new IllegalStateException("No skeleton for GBID " + gbid);
            } else if (!JeeSharedSubscriptionsMqttMessagingSkeleton.class.isInstance(skeleton.get())) {
                throw new IllegalStateException("Skeleton for GBID " + gbid
                        + " is not of type JeeSharedSubscriptionsMqttMessagingSkeleton");
            }
            JeeSharedSubscriptionsMqttMessagingSkeleton.class.cast(skeleton.get()).subscribeToSharedTopic();
        });
    }

    private void registerCallbackHandlers(JoynrRuntime joynrRuntime) {
        callbackHandlerDiscovery.forEach(callbackBean -> {
            joynrRuntime.registerStatelessAsyncCallback(callbackBean);
        });
    }

    private void registerProviders(Set<Bean<?>> serviceProviderBeans, JoynrRuntime runtime) {
        final int maxRetryCount = getJoynrInjector().getInstance(Key.get(Integer.class,
                                                                         Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES)));
        final int retryIntervalMs = getJoynrInjector().getInstance(Key.get(Integer.class,
                                                                           Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS)));
        final boolean awaitRegistration = getJoynrInjector().getInstance(Key.get(Boolean.class,
                                                                                 Names.named(JeeIntegrationPropertyKeys.PROPERTY_JEE_AWAIT_REGISTRATION)));
        Set<ProviderRegistrationSettingsFactory> providerSettingsFactories = getProviderRegistrationSettingsFactories();

        for (Bean<?> bean : serviceProviderBeans) {
            Class<?> beanClass = bean.getBeanClass();
            Class<?> serviceInterface = beanClass.getAnnotation(ServiceProvider.class).serviceInterface();
            Class<?> providerInterface = serviceProviderDiscovery.getProviderInterfaceFor(serviceInterface);
            if (logger.isDebugEnabled()) {
                logger.debug("Provider registration started: registering the bean {} as provider {} for service {}.",
                             bean,
                             providerInterface,
                             serviceInterface);
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
            String domain = null;

            for (ProviderRegistrationSettingsFactory factory : providerSettingsFactories) {
                if (factory.providesFor(serviceInterface, beanClass)) {
                    providerQos = factory.createProviderQos();
                    gbids = factory.createGbids();
                    domain = factory.createDomain();
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

            if (domain == null) {
                domain = getDomainForProvider(beanClass);
            }

            // register provider
            ProviderRegistrar registrar = runtime.getProviderRegistrar(domain, provider)
                                                 .withProviderQos(providerQos)
                                                 .withGbids(gbids);
            if (!awaitRegistration) {
                // do not wait for registration result and disable registration retries
                logger.debug("Provider registration: trigger registration, bean {}", bean);
                registrar.register();
                continue;
            }
            // await registration result and retry in case of errors
            if (!GlobalCapabilitiesDirectoryProvider.class.equals(providerInterface)) {
                // Do not use awaitGlobalregistration for interface GlobalCapabilitiesDirectory to avoid potential deadlock
                // in case its implementation depends on a Singleton that might not have been created before JoynrIntegrationBean
                //
                // GlobalCapabilitiesDirectory should not be combined with other globally registered providers in the same
                // runtime because successful deployment (registration of the other providers) cannot be guaranteed.
                // The registration of other providers will fail if:
                // - the GlobalCapabilitiesDirectory depends on a Singleton that has not yet been created.
                //   The provider registration will be blocked in a dead lock in this case.
                // - the GlobalCapabilitiesDirectory is not the first provider to be registered which only happens by chance.
                //   (there is no order for the provider registration)
                logger.debug("Provider registration: awaitGlobalRegistration, bean {}", bean);
                registrar.awaitGlobalRegistration();
            }
            int attempt = 1;
            while (true) {
                logger.debug("Provider registration: attempt #{}, bean {}", attempt, bean);
                Future<Void> registrationFuture = registrar.register();

                try {
                    try {
                        // wait for internal ttl (60.000 ms + 10.000 ms) + 1.000 ms, see ttl for add in LocalDiscoveryAggregator
                        registrationFuture.get((long) MessagingQos.DEFAULT_TTL + 11000);
                        logger.info("Provider registration succeeded: attempt #{}, bean {}.", attempt, bean);
                        break;
                    } catch (JoynrRuntimeException | ApplicationException e) {
                        if (attempt > maxRetryCount) {
                            logger.error("Provider registration failed, giving up: attempt #{}, bean {}.",
                                         attempt,
                                         bean,
                                         e);
                            throw new JoynrRuntimeException("Provider registration failed for bean " + bean, e);
                        }
                        logger.warn("Provider registration failed, retrying in {} ms...: attempt #{}, bean {} (error: {})",
                                    retryIntervalMs,
                                    attempt,
                                    bean,
                                    e.toString());
                        attempt++;
                        Thread.sleep(retryIntervalMs);
                    }
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    throw new JoynrRuntimeException("Provider registration failed for bean " + bean, e);
                }
            }

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
        if (!(getJoynrInjector().getInstance(Key.get(Boolean.class,
                                                     Names.named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS))))) {
            logger.info("Unregistering provider ", joynrRuntimeFactory.getLocalDomain());
            for (Object provider : registeredProviders) {
                try {
                    joynrRuntime.unregisterProvider(joynrRuntimeFactory.getLocalDomain(), provider);
                } catch (Exception e) {
                    logger.error("Error unregistering provider", e);
                }
            }
        }
        joynrRuntime.shutdown(false);
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
