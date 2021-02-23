/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.integration;

import static com.google.inject.util.Modules.override;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE;
import static io.joynr.messaging.MessagingPropertyKeys.PERSISTENCE_FILE;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS;
import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;

import java.io.File;
import java.util.Properties;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.provider.Promise;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ReplyContext;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import joynr.exceptions.ApplicationException;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testAbstractProvider;
import joynr.tests.testProvider;
import joynr.tests.testStatelessAsync;
import joynr.tests.testStatelessAsyncCallback;
import joynr.tests.testSync;

public class SharedSusbcriptionsTest {

    private String providerParticipantIdsPersistenceFile;
    private String consumerParticipantIdsPersistenceFile;
    private String providerPersistenceFile;
    private String consumerPersistenceFile;
    private Module runtimeModule;
    private String domain;
    private JoynrRuntime providerRuntime;
    private JoynrRuntime consumerRuntime;

    @Mock
    private testProvider providerMock;

    private testAbstractProvider provider = new DefaulttestProvider() {

        @Override
        public Promise<MethodWithStringsDeferred> methodWithStrings(String input) {
            providerMock.methodWithStrings(input);
            super.methodWithStrings(input); // just for logging
            MethodWithStringsDeferred deferred = new MethodWithStringsDeferred();
            deferred.resolve(input);
            return new Promise<>(deferred);
        }

    };

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        providerParticipantIdsPersistenceFile = "sst-provider-participantIds.persist";
        consumerParticipantIdsPersistenceFile = "sst-consumer-participantIds.persist";
        providerPersistenceFile = "sst-provider.persist";
        consumerPersistenceFile = "sst-consumer.persist";
        removePersistenceFiles();

        domain = "sst-domain-" + System.currentTimeMillis();
        runtimeModule = override(new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule());
    }

    private void removePersistenceFiles() {
        new File(providerParticipantIdsPersistenceFile).delete();
        new File(consumerParticipantIdsPersistenceFile).delete();
        new File(providerPersistenceFile).delete();
        new File(consumerPersistenceFile).delete();
    }

    @After
    public void tearDown() throws Exception {
        if (providerRuntime != null) {
            providerRuntime.unregisterProvider(domain, provider);
            providerRuntime.prepareForShutdown(); // wait some time for the global remove
            providerRuntime.shutdown(false);
        }
        if (consumerRuntime != null) {
            consumerRuntime.prepareForShutdown();
            consumerRuntime.shutdown(false);
        }
        removePersistenceFiles();
    }

    private void startProviderRuntime(Boolean enableSharedSubscriptions,
                                      Boolean separateConnections) throws InterruptedException, ApplicationException {
        Properties providerProperties = new Properties();
        providerProperties.put(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, enableSharedSubscriptions.toString());
        providerProperties.put(PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, separateConnections.toString());
        providerProperties.put("joynr.runtime.prepareforshutdowntimeout", "10");
        providerProperties.put(PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE, providerParticipantIdsPersistenceFile);
        providerProperties.put(PERSISTENCE_FILE, providerPersistenceFile);
        Module providerRuntimeModule = override(new JoynrPropertiesModule(providerProperties)).with(runtimeModule);
        Injector injector1 = Guice.createInjector(providerRuntimeModule);
        providerRuntime = injector1.getInstance(JoynrRuntime.class);
        providerRuntime.getProviderRegistrar(domain, provider).awaitGlobalRegistration().register().get(10000);
    }

    private void startConsumerRuntime(Boolean enableSharedSubscriptions, Boolean separateConnections) {
        Properties consumerProperties = new Properties();
        consumerProperties.put(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, enableSharedSubscriptions.toString());
        consumerProperties.put(PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, separateConnections.toString());
        consumerProperties.put("joynr.runtime.prepareforshutdowntimeout", "10");
        consumerProperties.put(PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE, consumerParticipantIdsPersistenceFile);
        consumerProperties.put(PERSISTENCE_FILE, consumerPersistenceFile);
        Module consumerRuntimeModule = override(new JoynrPropertiesModule(consumerProperties)).with(runtimeModule);
        Injector injector2 = Guice.createInjector(consumerRuntimeModule);
        consumerRuntime = injector2.getInstance(JoynrRuntime.class);
    }

    private void buildAndCallSyncProxy() throws Exception {
        ProxyBuilder<testSync> builder = consumerRuntime.getProxyBuilder(domain, testSync.class);
        Future<Void> proxyFuture = new Future<Void>();
        testSync proxy = builder.build(new ProxyCreatedCallback<testSync>() {
            @Override
            public void onProxyCreationFinished(testSync result) {
                proxyFuture.onSuccess(null);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                proxyFuture.onFailure(error);
            }
        });
        proxyFuture.get(10000);

        String input = "sst-payload-" + System.currentTimeMillis();
        String result = proxy.methodWithStrings(input);

        assertEquals(input, result);
        verify(providerMock).methodWithStrings(eq(input));
    }

    @Test
    public void rpc_sharedSubscriptionsEnabledForConsumerAndProvider() throws Exception {
        startProviderRuntime(true, false);
        startConsumerRuntime(true, false);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_sharedSubscriptionsEnabledForProviderOnly() throws Exception {
        startProviderRuntime(true, false);
        startConsumerRuntime(false, false);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_sharedSubscriptionsEnabledForConsumerOnly() throws Exception {
        startProviderRuntime(false, false);
        startConsumerRuntime(true, false);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_sharedSubscriptionsDisabled() throws Exception {
        startProviderRuntime(false, false);
        startConsumerRuntime(false, false);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_separateConnections_sharedSubscriptionsEnabledForConsumerAndProvider() throws Exception {
        startProviderRuntime(true, true);
        startConsumerRuntime(true, true);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_separateConnections_sharedSubscriptionsEnabledForProviderOnly() throws Exception {
        startProviderRuntime(true, true);
        startConsumerRuntime(false, true);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_separateConnections_sharedSubscriptionsEnabledForConsumerOnly() throws Exception {
        startProviderRuntime(false, true);
        startConsumerRuntime(true, true);
        buildAndCallSyncProxy();
    }

    @Test
    public void rpc_separateConnections_sharedSubscriptionsDisabled() throws Exception {
        startProviderRuntime(false, true);
        startConsumerRuntime(false, true);
        buildAndCallSyncProxy();
    }

    public class StatelessCallback implements testStatelessAsyncCallback {
        private String useCase;
        Future<String> messageIdFuture;
        Future<String> resultFuture;

        public StatelessCallback(String useCase, Future<String> messageIdFuture, Future<String> resultFuture) {
            this.useCase = useCase;
            this.messageIdFuture = messageIdFuture;
            this.resultFuture = resultFuture;
        }

        @Override
        public String getUseCase() {
            return useCase;
        }

        @Override
        public void methodWithStringsSuccess(String result, ReplyContext replyContext) {
            try {
                assertEquals(messageIdFuture.get(10000), replyContext.getMessageId());
                resultFuture.onSuccess(result);
            } catch (Exception e) {
                resultFuture.onFailure(new JoynrRuntimeException("Failed to compare message IDs: " + e));
            }
        }

        @Override
        public void methodWithStringsFailed(JoynrRuntimeException runtimeException, ReplyContext replyContext) {
            resultFuture.onFailure(new JoynrRuntimeException("methodWithStringsFailed: " + runtimeException));
        }
    }

    private void buildAndCallStatelessAsyncProxy() throws Exception {
        String useCase = "sst-useCase-" + System.currentTimeMillis();
        Future<String> messageIdFuture = new Future<String>();
        Future<String> resultFuture = new Future<String>();

        testStatelessAsyncCallback callback = new StatelessCallback(useCase, messageIdFuture, resultFuture);
        consumerRuntime.registerStatelessAsyncCallback(callback);

        ProxyBuilder<testStatelessAsync> builder = consumerRuntime.getProxyBuilder(domain, testStatelessAsync.class);
        Future<Void> proxyFuture = new Future<Void>();
        testStatelessAsync proxy = builder.setStatelessAsyncCallbackUseCase(useCase)
                                          .build(new ProxyCreatedCallback<testStatelessAsync>() {
                                              @Override
                                              public void onProxyCreationFinished(testStatelessAsync result) {
                                                  proxyFuture.onSuccess(null);
                                              }

                                              @Override
                                              public void onProxyCreationError(JoynrRuntimeException error) {
                                                  proxyFuture.onFailure(error);
                                              }
                                          });
        proxyFuture.get(10000);

        String input = "sst-payload-" + System.currentTimeMillis();
        proxy.methodWithStrings(input, id -> messageIdFuture.onSuccess(id));

        String result = resultFuture.get(10000);
        assertEquals(input, result);
        verify(providerMock).methodWithStrings(eq(input));
    }

    @Test
    public void statelessAsync_sharedSubscriptionsEnabledForConsumerAndProvider() throws Exception {
        startProviderRuntime(true, true);
        startConsumerRuntime(true, true);
        buildAndCallStatelessAsyncProxy();
    }

    @Test
    public void statelessAsync_sharedSubscriptionsEnabledForProviderOnly() throws Exception {
        startProviderRuntime(true, true);
        startConsumerRuntime(false, true);
        buildAndCallStatelessAsyncProxy();
    }

    @Test
    public void statelessAsync_sharedSubscriptionsEnabledForConsumerOnly() throws Exception {
        startProviderRuntime(false, true);
        startConsumerRuntime(true, true);
        buildAndCallStatelessAsyncProxy();
    }

    @Test
    public void statelessAsync_sharedSubscriptionsDisabled() throws Exception {
        startProviderRuntime(false, true);
        startConsumerRuntime(false, true);
        buildAndCallStatelessAsyncProxy();
    }

}
