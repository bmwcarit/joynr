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
package io.joynr.integration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.provider.Promise;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ProviderRegistrar;
import joynr.ImmutableMessage;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.infrastructure.GlobalDomainAccessControlListEditorProxy;
import joynr.infrastructure.GlobalDomainRoleControllerProxy;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class AccessControllerEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(AccessControllerEnd2EndTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    static final long CONST_GLOBAL_TEST_TIMEOUT_MS = 120000;
    private static final String TEST_DOMAIN = "test";
    private static final String GDAC_DOMAIN = "io.joynr";
    private static final long DISCOVERY_TIMEOUT = 14000;
    private static final long MESSAGING_TTL = 15000;

    private JoynrRuntime runtime;
    private TestProviderImpl testProvider;

    @Rule
    public ExpectedException expectedException = ExpectedException.none();

    private static class TestProviderImpl extends DefaulttestProvider {
        public Promise<AddNumbersDeferred> addNumbers(Integer first, Integer second, Integer third) {
            AddNumbersDeferred addNumbersDeferred = new AddNumbersDeferred();
            addNumbersDeferred.resolve(first + second + third);

            return new Promise<AddNumbersDeferred>(addNumbersDeferred);
        }
    }

    @Before
    public void setUp() {
        runtime = createRuntime();
        registerProvider(runtime);
    }

    @After
    public void tearDown() {
        if (testProvider != null) {
            runtime.unregisterProvider(TEST_DOMAIN, testProvider);
            testProvider = null;
        }
        runtime.shutdown(true);
    }

    @Test(timeout = CONST_GLOBAL_TEST_TIMEOUT_MS)
    public void testAllowedRPCCallSucceeds() {
        createDefaultGDACEntries(TEST_DOMAIN,
                                 testProxy.INTERFACE_NAME,
                                 "*",
                                 ImmutableMessage.DUMMY_CREATOR_USER_ID,
                                 Permission.YES);

        testProxy testProxy = createProxy(runtime);

        Integer result = testProxy.addNumbers(3, 5, 7);

        assertEquals(15, result.intValue());
    }

    @Test(timeout = CONST_GLOBAL_TEST_TIMEOUT_MS)
    public void testForbiddenRPCCallFails() {
        createDefaultGDACEntries(TEST_DOMAIN,
                                 testProxy.INTERFACE_NAME,
                                 "*",
                                 ImmutableMessage.DUMMY_CREATOR_USER_ID,
                                 Permission.NO);

        testProxy testProxy = createProxy(runtime);

        expectedException.expect(JoynrCommunicationException.class);
        testProxy.addNumbers(3, 5, 7);
    }

    private JoynrRuntime createRuntime() {
        Properties properties = new Properties();

        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:1883");
        properties.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");

        Module module = Modules.override(new CCInProcessRuntimeModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                bindConstant().annotatedWith(Names.named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE))
                              .to(true);
            }
        }, new HivemqMqttClientModule());

        DummyJoynrApplication app = (DummyJoynrApplication) new JoynrInjectorFactory(properties,
                                                                                     module).createApplication(DummyJoynrApplication.class);

        return app.getRuntime();
    }

    private void registerProvider(JoynrRuntime runtime) {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        providerQos.setPriority(System.currentTimeMillis());

        testProvider = new TestProviderImpl();

        runtime.getProviderRegistrar(TEST_DOMAIN, testProvider).withProviderQos(providerQos).register();
    }

    private testProxy createProxy(JoynrRuntime runtime) {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT);

        MessagingQos messagingQos = new MessagingQos();
        messagingQos.setTtl_ms(MESSAGING_TTL);

        final Future<Void> future = new Future<Void>();
        testProxy testProxy;

        testProxy = runtime.getProxyBuilder(TEST_DOMAIN, testProxy.class)
                           .setDiscoveryQos(discoveryQos)
                           .setMessagingQos(messagingQos)
                           .build(new ProxyCreatedCallback<testProxy>() {
                               @Override
                               public void onProxyCreationFinished(testProxy result) {
                                   future.onSuccess(null);
                               }

                               @Override
                               public void onProxyCreationError(JoynrRuntimeException error) {
                                   future.onFailure(error);
                               }
                           });
        try {
            future.get(5000);
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
        return testProxy;
    }

    private void createDefaultGDACEntries(String domainName,
                                          String interfaceName,
                                          String operationName,
                                          String userId,
                                          Permission permission) {
        MasterAccessControlEntry testEntry = new MasterAccessControlEntry();
        testEntry.setUid(userId);
        testEntry.setDomain(domainName);
        testEntry.setInterfaceName(interfaceName);
        testEntry.setDefaultConsumerPermission(permission);
        testEntry.setOperation(operationName);
        testEntry.setPossibleConsumerPermissions(new Permission[]{ Permission.YES, Permission.NO });
        testEntry.setPossibleRequiredTrustLevels(new TrustLevel[]{ testEntry.getDefaultRequiredTrustLevel() });

        List<MasterAccessControlEntry> provisionedACEs = new ArrayList<MasterAccessControlEntry>();
        provisionedACEs.add(testEntry);

        DomainRoleEntry domainMasterRoleEntry = new DomainRoleEntry();
        domainMasterRoleEntry.setDomains(new String[]{ domainName });
        domainMasterRoleEntry.setRole(Role.MASTER);
        domainMasterRoleEntry.setUid(userId);

        DomainRoleEntry domainOwnerRoleEntry = new DomainRoleEntry();
        domainOwnerRoleEntry.setDomains(new String[]{ domainName });
        domainOwnerRoleEntry.setRole(Role.OWNER);
        domainOwnerRoleEntry.setUid(userId);

        List<DomainRoleEntry> domainRoleEntries = new ArrayList<DomainRoleEntry>();
        domainRoleEntries.add(domainMasterRoleEntry);
        domainRoleEntries.add(domainOwnerRoleEntry);

        OwnerAccessControlEntry ownerControlEntry = new OwnerAccessControlEntry();
        ownerControlEntry.setDomain(domainName);
        ownerControlEntry.setInterfaceName(interfaceName);
        ownerControlEntry.setUid(userId);
        ownerControlEntry.setConsumerPermission(Permission.YES);

        List<OwnerAccessControlEntry> ownerEntries = new ArrayList<OwnerAccessControlEntry>();
        ownerEntries.add(ownerControlEntry);

        sendProvisionedEntriesToGDAC(domainRoleEntries, provisionedACEs, provisionedACEs, ownerEntries);
    }

    private void sendProvisionedEntriesToGDAC(List<DomainRoleEntry> domainRoleEntries,
                                              List<MasterAccessControlEntry> masterAccessControlEntries,
                                              List<MasterAccessControlEntry> mediatorAccessControlEntries,
                                              List<OwnerAccessControlEntry> ownerAccessControlEntries) {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryTimeoutMs(60000);
        discoveryQos.setRetryIntervalMs(10000);

        MessagingQos messagingQos = new MessagingQos();
        messagingQos.setTtl_ms(60000);

        GlobalDomainAccessControlListEditorProxy gdacListEditorProxy = runtime.getProxyBuilder(GDAC_DOMAIN,
                                                                                               GlobalDomainAccessControlListEditorProxy.class)
                                                                              .setDiscoveryQos(discoveryQos)
                                                                              .setMessagingQos(messagingQos)
                                                                              .build();

        GlobalDomainRoleControllerProxy gdrcProxy = runtime.getProxyBuilder(GDAC_DOMAIN,
                                                                            GlobalDomainRoleControllerProxy.class)
                                                           .setDiscoveryQos(discoveryQos)
                                                           .setMessagingQos(messagingQos)
                                                           .build();

        for (DomainRoleEntry entry : domainRoleEntries) {
            assertTrue(gdrcProxy.updateDomainRole(entry));
        }

        for (MasterAccessControlEntry entry : masterAccessControlEntries) {
            assertTrue(gdacListEditorProxy.updateMasterAccessControlEntry(entry));
        }

        for (MasterAccessControlEntry entry : mediatorAccessControlEntries) {
            assertTrue(gdacListEditorProxy.updateMediatorAccessControlEntry(entry));
        }

        for (OwnerAccessControlEntry entry : ownerAccessControlEntries) {
            assertTrue(gdacListEditorProxy.updateOwnerAccessControlEntry(entry));
        }
    }
}
