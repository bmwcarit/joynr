/*
 * #%L
 * %%
 * Copyright (C) 2024 BMW Car IT GmbH
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

import static com.google.inject.util.Modules.override;
import static java.lang.String.format;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.runtime.ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE;

import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.function.Supplier;

import jakarta.annotation.Resource;
import jakarta.ejb.DependsOn;
import jakarta.ejb.Singleton;
import jakarta.enterprise.inject.Any;
import jakarta.enterprise.inject.Instance;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.enterprise.util.AnnotationLiteral;
import jakarta.inject.Inject;

import com.fasterxml.jackson.databind.jsontype.impl.LaissezFaireSubTypeValidator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;

import io.joynr.ProvidedBy;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys;
import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrMqttClientIdProvider;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.jeeintegration.api.JoynrRawMessagingPreprocessor;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.provider.JoynrInterface;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.statusmetrics.JoynrStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

/**
 * Default implementation for {@link JoynrRuntimeFactory}, which will use information produced by
 * {@link JoynrProperties} and {@link JoynrLocalDomain}, if available, to configure
 * the joynr runtime and application with.
 * <p>
 * <b>IMPORTANT</b>: This class requires the EE runtime to have been configured with a ManagedScheduledExecutorService
 * resource which has been given the name 'concurrent/joynrMessagingScheduledExecutor'.
 */
@DependsOn("JeeJoynrStatusMetricsAggregator")
@Singleton
public class DefaultJoynrRuntimeFactory implements JoynrRuntimeFactory {

    private static final Logger logger = LoggerFactory.getLogger(DefaultJoynrRuntimeFactory.class);

    private final Properties joynrProperties;

    private final String joynrLocalDomain;

    private final BeanManager beanManager;

    private final JoynrStatusMetricsReceiver joynrStatusMetrics;

    /**
     * The scheduled executor service to use for providing to the joynr runtime.
     */
    @Resource(name = JeeIntegrationPropertyKeys.JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE)
    private ScheduledExecutorService scheduledExecutorService;

    private Injector fInjector = null;

    private RawMessagingPreprocessor rawMessagePreprocessor;
    private MqttClientIdProvider mqttClientIdProvider;

    public static final String ERROR_NO_LOCAL_DOMAIN = "No local domain name specified. Please provide a value for "
            + "the local domain via @JoynrLocalDomain in your configuration EJB.";
    public static final String ERROR_MULTIPLE_LOCAL_DOMAINS = "Multiple local domain names specified. Please provide "
            + "only one configuration EJB containing a value for the local domain via @JoynrLocalDomain.";
    public static final String ERROR_LOCAL_DOMAIN_IS_EMPTY = "Local domain name is NULL or EMPTY. Please provide "
            + "a value for the local domain via @JoynrLocalDomain in your configuration EJB.";
    public static final String ERROR_MULTIPLE_PREPROCESSORS = "Only one RawMessagePreprocessor may be provided.";
    public static final String ERROR_MULTIPLE_ID_CLIENTS = "Only one MqttClientIdProvider may be provided.";
    public static final String ERROR_MULTIPLE_PROPERTIES = "Multiple joynrProperties specified. Please provide "
            + "only one configuration EJB containing a value for the joynrProperties via @JoynrProperties.";

    /**
     * Constructor in which the JEE runtime injects the managed resources and the JEE joynr integration specific
     * configuration data (see {@link JoynrProperties} and {@link JoynrLocalDomain}
     * ).
     * <p>
     * <b>Note</b> that if the EJB which contains the producer methods implements an interface, then the producer
     * methods also need to be declared in that interface, otherwise CDI won't recognise the method implementations as
     * producers.
     *
     * @param joynrProperties  the joynr properties, if present, by {@link #prepareJoynrProperties(Properties)} to
     * prepare the properties with which the injector is created.
     * @param joynrLocalDomain the joynr local domain name to use for the application.
     * @param rawMessagePreprocessor can be optionally provided to intercept incoming messages and inspect or modify them
     * @param mqttClientIdProvider can be optionally provided to generate custom mqtt client id
     * @param beanManager bean manager
     * @param joynrStatusMetrics Is passed to POJO joynr and receives metrics about the status of the mqtt connection.
     */
    // CHECKSTYLE:OFF
    @Inject
    public DefaultJoynrRuntimeFactory(final @JoynrProperties Instance<Properties> joynrProperties,
                                      final @JoynrLocalDomain Instance<String> joynrLocalDomain,
                                      final @JoynrRawMessagingPreprocessor Instance<RawMessagingPreprocessor> rawMessagePreprocessor,
                                      final @JoynrMqttClientIdProvider Instance<MqttClientIdProvider> mqttClientIdProvider,
                                      final BeanManager beanManager,
                                      final JoynrStatusMetricsReceiver joynrStatusMetrics) {
        // CHECKSTYLE:ON
        if (joynrLocalDomain.isUnsatisfied()) {
            logErrorAndThrow(ERROR_NO_LOCAL_DOMAIN);
        } else if (joynrLocalDomain.isAmbiguous()) {
            logErrorAndThrow(ERROR_MULTIPLE_LOCAL_DOMAINS);
        }
        this.joynrLocalDomain = joynrLocalDomain.get();
        if (this.joynrLocalDomain == null || this.joynrLocalDomain.isEmpty()) {
            logErrorAndThrow(ERROR_LOCAL_DOMAIN_IS_EMPTY);
        }

        if (!rawMessagePreprocessor.isUnsatisfied()) {
            if (!rawMessagePreprocessor.isAmbiguous()) {
                this.rawMessagePreprocessor = rawMessagePreprocessor.get();
            } else {
                logErrorAndThrow(ERROR_MULTIPLE_PREPROCESSORS);
            }
        } else {
            this.rawMessagePreprocessor = new NoOpRawMessagingPreprocessor();
        }

        if (!mqttClientIdProvider.isUnsatisfied()) {
            if (!mqttClientIdProvider.isAmbiguous()) {
                this.mqttClientIdProvider = mqttClientIdProvider.get();
            } else {
                logErrorAndThrow(ERROR_MULTIPLE_ID_CLIENTS);
            }
        } else {
            this.mqttClientIdProvider = null;
        }

        Properties configuredProperties = new Properties();
        if (!joynrProperties.isUnsatisfied()) {
            if (!joynrProperties.isAmbiguous()) {
                configuredProperties = joynrProperties.get();
                logger.info("Got custom joynr properties: {}", configuredProperties);
            } else {
                logErrorAndThrow(ERROR_MULTIPLE_PROPERTIES);
            }
        } else {
            logger.info("No custom joynr properties provided. Will use default properties.");
        }
        this.joynrProperties = prepareJoynrProperties(configuredProperties);
        this.beanManager = beanManager;
        this.joynrStatusMetrics = joynrStatusMetrics;
    }

    @Override
    public JoynrRuntime create(final Set<Class<?>> providerInterfaceClasses) {
        logger.info("Creating clusterable participant IDs for discovered providers.");
        createClusterableParticipantIds(providerInterfaceClasses);
        logger.info("Provisioning access control for {}", providerInterfaceClasses);
        provisionAccessControl(joynrProperties, joynrLocalDomain, getProviderInterfaceNames(providerInterfaceClasses));
        logger.info(format("Creating application with joynr properties:%n%s", joynrProperties));
        final JoynrRuntime runtime = getInjector().getInstance(JoynrRuntime.class);
        logger.info("Created runtime: {}", runtime);
        return runtime;
    }

    private void createClusterableParticipantIds(final Set<Class<?>> providerInterfaceClasses) {
        for (final Class<?> joynrProviderClass : providerInterfaceClasses) {
            final String participantIdKey = ParticipantIdKeyUtil.getProviderParticipantIdKey(getLocalDomain(),
                                                                                             joynrProviderClass);
            if (!joynrProperties.containsKey(participantIdKey)) {
                joynrProperties.put(participantIdKey, createClusterableParticipantId(joynrProviderClass));
            }
        }
    }

    private String createClusterableParticipantId(final Class<?> joynrProviderClass) {
        final String key = getLocalDomain() + "." + joynrProperties.getProperty(MessagingPropertyKeys.CHANNELID) + "."
                + getInterfaceName(joynrProviderClass);
        return key.replace("/", ".");
    }

    @Override
    public Injector getInjector() {
        if (fInjector == null) {
            final Module jeeModule = override(new CCInProcessRuntimeModule()).with(new JeeJoynrIntegrationModule(scheduledExecutorService));
            final Module finalModule = override(jeeModule).with(new AbstractModule() {
                @Override
                protected void configure() {
                    bind(RawMessagingPreprocessor.class).toInstance(rawMessagePreprocessor);

                    if (mqttClientIdProvider != null) {
                        bind(MqttClientIdProvider.class).toInstance(mqttClientIdProvider);
                    }

                    bind(JoynrStatusMetrics.class).toInstance(joynrStatusMetrics);
                    bind(JoynrStatusMetricsReceiver.class).toInstance(joynrStatusMetrics);
                }
            });

            fInjector = new JoynrInjectorFactory(joynrProperties,
                                                 getMessageProcessorsModule(),
                                                 finalModule).getInjector();
        }
        return fInjector;
    }

    private AbstractModule getMessageProcessorsModule() {
        return getModuleForBeansOfType(JoynrMessageProcessor.class, () -> new TypeLiteral<>() {
        });
    }

    private <T> AbstractModule getModuleForBeansOfType(final Class<T> beanType,
                                                       final Supplier<TypeLiteral<T>> typeLiteralSupplier) {
        final Set<Bean<?>> beans = beanManager.getBeans(beanType, new AnnotationLiteral<Any>() {
            private static final long serialVersionUID = 1L;
        });
        return new AbstractModule() {
            @SuppressWarnings("unchecked")
            @Override
            protected void configure() {
                final Multibinder<T> beanMultibinder = Multibinder.newSetBinder(binder(), typeLiteralSupplier.get());
                for (final Bean<?> bean : beans) {
                    beanMultibinder.addBinding()
                                   .toInstance((T) Proxy.newProxyInstance(getClass().getClassLoader(),
                                                                          new Class[]{ beanType },
                                                                          new BeanCallingProxy<>((Bean<T>) bean,
                                                                                                 beanManager)));
                }
            }
        };
    }

    private Properties prepareJoynrProperties(final Properties configuredProperties) {
        final Properties defaultJoynrProperties = new Properties();
        defaultJoynrProperties.setProperty(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, joynrLocalDomain);
        defaultJoynrProperties.putAll(configuredProperties);
        if (!defaultJoynrProperties.containsKey(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS)) {
            logger.info("Shared Subscriptions Option is explicitly set to true.");
            defaultJoynrProperties.setProperty(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "true");
        } else {
            logger.info("Shared Subscriptions Option is specified in properties: {}.",
                        defaultJoynrProperties.getProperty(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS));
        }

        return defaultJoynrProperties;
    }

    private String[] getProviderInterfaceNames(final Set<Class<?>> providerInterfaceClasses) {
        return providerInterfaceClasses.stream().map(this::getInterfaceName).toArray(String[]::new);
    }

    private String getInterfaceName(final Class<?> providerInterfaceClass) {
        try {
            final ProvidedBy providedBy = providerInterfaceClass.getAnnotation(ProvidedBy.class);
            final JoynrInterface joynrInterface = providedBy.value().getAnnotation(JoynrInterface.class);
            return joynrInterface.name() + ".v" + ProviderAnnotations.getMajorVersion(providedBy.value());
        } catch (final SecurityException | IllegalArgumentException e) {
            logger.debug("error getting interface details", e);
            return providerInterfaceClass.getSimpleName();
        }
    }

    private void provisionAccessControl(final Properties properties,
                                        final String domain,
                                        final String[] interfaceNames) {
        final boolean enableAccessControl = Boolean.parseBoolean(properties.getProperty(PROPERTY_ACCESSCONTROL_ENABLE,
                                                                                        Boolean.FALSE.toString()));
        if (!enableAccessControl) {
            // Nothing to do
            return;
        }
        final ObjectMapper objectMapper = new ObjectMapper();
        //noinspection deprecation
        objectMapper.activateDefaultTypingAsProperty(LaissezFaireSubTypeValidator.instance,
                                                     DefaultTyping.JAVA_LANG_OBJECT,
                                                     "_typeName");

        final MasterAccessControlEntry[] provisionedAccessControlEntries = Arrays.stream(interfaceNames)
                                                                                 .map(interfaceName -> createMasterAccessControlEntry(interfaceName,
                                                                                                                                      domain))
                                                                                 .toArray(MasterAccessControlEntry[]::new);
        String provisionedAccessControlEntriesAsJson;
        try {
            provisionedAccessControlEntriesAsJson = objectMapper.writeValueAsString(provisionedAccessControlEntries);
            properties.setProperty(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES,
                                   provisionedAccessControlEntriesAsJson);
        } catch (JsonProcessingException e) {
            logger.error("Error parsing JSON.", e);
        }
    }

    private MasterAccessControlEntry createMasterAccessControlEntry(final String interfaceName, final String domain) {
        return new MasterAccessControlEntry("*",
                                            domain,
                                            interfaceName,
                                            TrustLevel.LOW,
                                            new TrustLevel[]{ TrustLevel.LOW },
                                            TrustLevel.LOW,
                                            new TrustLevel[]{ TrustLevel.LOW },
                                            "*",
                                            Permission.YES,
                                            new Permission[]{ joynr.infrastructure.DacTypes.Permission.YES });
    }

    @Override
    public String getLocalDomain() {
        return joynrLocalDomain;
    }

    public RawMessagingPreprocessor getRawMessagePreprocessor() {
        return rawMessagePreprocessor;
    }

    public MqttClientIdProvider getMqttClientIdProvider() {
        return mqttClientIdProvider;
    }

    private void logErrorAndThrow(final String message) {
        logger.error(message);
        throw new JoynrIllegalStateException(message);
    }
}
