/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

package io.joynr.performance;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.performance.EchoProviderInvocationParameters.BackendConfig;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.tests.performance.EchoProvider;
import joynr.types.ProviderQos;

public class EchoProviderApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(EchoProviderApplication.class);
    private static final String STATIC_PERSISTENCE_FILE = "provider-joynr.properties";
    private static EchoProviderInvocationParameters invocationParams = null;

    private EchoProvider provider = null;

    public static void main(String[] args) {
        try {
            invocationParams = new EchoProviderInvocationParameters(args);

            JoynrApplication joynrApplication = createJoynrApplication();

            joynrApplication.run();
            joynrApplication.shutdown();
        } catch (Exception exception) {
            System.err.println(exception.getMessage());
            System.exit(1);
        }
    }

    @Override
    public void run() {
        provider = new EchoProviderImpl();

        ProviderQos providerQos = new ProviderQos();

        // Ensure that the most recent provider has got the highest priority
        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(invocationParams.getProviderScope());

        runtime.registerProvider(localDomain, provider, providerQos);

        // At the moment the provider can only be stopped by using ctrl-c or 'kill'.
        // Console input does not work if the provider is started asynchronously by maven.
        // In this case the application appears to crash.
        try {
            Thread.sleep(24 * 60 * 60 * 1000); // 24h
        } catch (InterruptedException e) {
            // Can be ignored in this context
        }
    }

    @Override
    public void shutdown() {
        if (null != provider) {
            try {
                runtime.unregisterProvider(localDomain, provider);
            } catch (JoynrRuntimeException exception) {
                logger.error("Failed to unregister provider", exception);
            }
        }

        runtime.shutdown(true);
        System.exit(0);
    }

    private static JoynrApplication createJoynrApplication() throws Exception {
        Module runtimeModule = Modules.override(getRuntimeModule()).with(getBackendModule());

        Properties joynrConfig = createJoynrConfig();
        Properties appConfig = createAppConfig();

        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig,
                                                                        runtimeModule,
                                                                        new StaticDomainAccessControlProvisioningModule());

        JoynrApplication joynrApplication = injectorFactory.createApplication(new JoynrApplicationModule(EchoProviderApplication.class,
                                                                                                         appConfig));

        return joynrApplication;
    }

    private static Properties createJoynrConfig() throws Exception {
        Properties joynrConfig = new Properties();

        if (invocationParams.getBackendTransportMode() == BackendConfig.MQTT) {
            joynrConfig.put("joynr.messaging.mqtt.brokerUri", invocationParams.getMqttBrokerUri());
            joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        }

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, invocationParams.getDomain());

        return joynrConfig;
    }

    private static Properties createAppConfig() throws Exception {
        Properties appConfig = new Properties();

        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry("*",
                                                                                            invocationParams.getDomain(),
                                                                                            ProviderAnnotations.getInterfaceName(EchoProviderImpl.class),
                                                                                            TrustLevel.LOW,
                                                                                            new TrustLevel[]{
                                                                                                    TrustLevel.LOW },
                                                                                            TrustLevel.LOW,
                                                                                            new TrustLevel[]{
                                                                                                    TrustLevel.LOW },
                                                                                            "*",
                                                                                            Permission.YES,
                                                                                            new Permission[]{
                                                                                                    Permission.YES });

        MasterAccessControlEntry[] provisionedAccessControlEntries = { newMasterAccessControlEntry };
        String provisionedAccessControlEntriesAsJson = objectMapper.writeValueAsString(provisionedAccessControlEntries);
        appConfig.setProperty(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES,
                              provisionedAccessControlEntriesAsJson);

        return appConfig;
    }

    private static Module getRuntimeModule() throws Exception {
        switch (invocationParams.getRuntimeMode()) {
        case IN_PROCESS_CC:
            return new CCInProcessRuntimeModule();
        default:
            throw new Exception("Unknown runtime requested");
        }
    }

    private static Module getBackendModule() throws Exception {
        Module backendTransportModules = Modules.EMPTY_MODULE;
        switch (invocationParams.getBackendTransportMode()) {
        case MQTT:
            return Modules.combine(backendTransportModules, new HivemqMqttClientModule());
        default:
            throw new Exception("Unknown backend requested");
        }
    }
}
