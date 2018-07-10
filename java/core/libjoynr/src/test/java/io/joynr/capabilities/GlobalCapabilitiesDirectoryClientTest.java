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
package io.joynr.capabilities;

import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyListOf;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.GlobalDiscoveryEntry;

@RunWith(MockitoJUnitRunner.class)
public class GlobalCapabilitiesDirectoryClientTest {
    @Mock
    private ProxyBuilder<GlobalCapabilitiesDirectoryProxy> capabilitiesProxyBuilderMock;

    @Mock
    private GlobalCapabilitiesDirectoryProxy globalCapabilitiesDirectoryProxyMock;

    @Mock
    private Callback<Void> callbackMock;

    @Mock
    GlobalDiscoveryEntry capabilitiesDirectoryEntryMock;

    @Mock
    private ProxyBuilderFactory proxyBuilderFactoryMock;

    @Captor
    private ArgumentCaptor<Callback<GlobalDiscoveryEntry[]>> callbackArrayOfGlobalDiscoveryEntryCaptor;

    private GlobalCapabilitiesDirectoryClient subject;

    private static final long DEFAULT_TTL_ADD_AND_REMOVE = 60L * 1000L;

    private static final long CUSTOM_TTL = 3L * 1000L;

    private static final long FRESHNESS_UPDATE_INTERVAL_MS = 42;

    private final MessagingQos messagingQos = new MessagingQos();

    @Before
    public void setup() {
        final String domainMock = "domainMock";
        when(capabilitiesDirectoryEntryMock.getDomain()).thenReturn(domainMock);

        Properties properties = new Properties();
        properties.put(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS,
                       String.valueOf(FRESHNESS_UPDATE_INTERVAL_MS));

        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            public void configure() {
                bind(ProxyBuilderFactory.class).toInstance(proxyBuilderFactoryMock);
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntryMock);
            }
        }, new JoynrPropertiesModule(properties));
        subject = injector.getInstance(GlobalCapabilitiesDirectoryClient.class);

        when(proxyBuilderFactoryMock.get(domainMock,
                                         GlobalCapabilitiesDirectoryProxy.class)).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setDiscoveryQos(any(DiscoveryQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setMessagingQos(any(MessagingQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.build()).thenReturn(globalCapabilitiesDirectoryProxyMock);
    }

    @Test
    public void testAdd() {
        messagingQos.setTtl_ms(DEFAULT_TTL_ADD_AND_REMOVE);
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);
        subject.add(callbackMock, capabilitiesDirectoryEntryMock);
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackMock), eq(capabilitiesDirectoryEntryMock));
    }

    @Test
    public void testRemove() {
        messagingQos.setTtl_ms(DEFAULT_TTL_ADD_AND_REMOVE);
        List<String> testParticipantIdList = new ArrayList<String>();
        final String testParticipantId = "testParticipantId";
        testParticipantIdList.add(testParticipantId);
        subject.remove(callbackMock, testParticipantIdList);
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        String[] testParticipantIdToArray = testParticipantIdList.toArray(new String[testParticipantIdList.size()]);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock), eq(testParticipantIdToArray));
    }

    private GlobalCapabilitiesDirectoryClient getClientWithCustomTTL(long ttl) {
        Properties properties = new Properties();
        properties.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS,
                       String.valueOf(ttl));

        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            public void configure() {
                bind(ProxyBuilderFactory.class).toInstance(proxyBuilderFactoryMock);
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntryMock);
            }
        }, new JoynrPropertiesModule(properties));

        return injector.getInstance(GlobalCapabilitiesDirectoryClient.class);
    }

    @Test
    public void testAddWithCustomTTL() {
        GlobalCapabilitiesDirectoryClient subjectInject = getClientWithCustomTTL(CUSTOM_TTL);
        messagingQos.setTtl_ms(CUSTOM_TTL);
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);
        subjectInject.add(callbackMock, capabilitiesDirectoryEntryMock);
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackMock), eq(capabilitiesDirectoryEntryMock));
    }

    @Test
    public void testRemoveWithCustomTTL() {
        GlobalCapabilitiesDirectoryClient subjectInject = getClientWithCustomTTL(CUSTOM_TTL);
        messagingQos.setTtl_ms(CUSTOM_TTL);
        List<String> testParticipantIdList = new ArrayList<String>();
        final String testParticipantId = "testParticipantId";
        testParticipantIdList.add(testParticipantId);
        subjectInject.remove(callbackMock, testParticipantIdList);
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        String[] testParticipantIdToArray = testParticipantIdList.toArray(new String[testParticipantIdList.size()]);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock), eq(testParticipantIdToArray));
    }

    @Test
    public void testLookupParticipantId() {
        @SuppressWarnings("unchecked")
        Callback<GlobalDiscoveryEntry> callbackGlobalDiscoveryEntryMock = (Callback<GlobalDiscoveryEntry>) mock(Callback.class);
        messagingQos.setTtl_ms(CUSTOM_TTL);
        final String testParticipantId = "testParticipantId";
        subject.lookup(callbackGlobalDiscoveryEntryMock, testParticipantId, CUSTOM_TTL);
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        verify(globalCapabilitiesDirectoryProxyMock).lookup(eq(callbackGlobalDiscoveryEntryMock),
                                                            eq(testParticipantId));
    }

    private Callback<GlobalDiscoveryEntry[]> lookupDomainsHelper(Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock) {
        String[] domainsStrArrayDummy = new String[]{ "dummyDomain1", "dummyDomain2", "dummyDomain3" };
        String interfaceNameDummy = "interfaceNameDummy";
        messagingQos.setTtl_ms(CUSTOM_TTL);
        subject.lookup(callbackListOfGlobalDiscoveryEntriesMock, domainsStrArrayDummy, interfaceNameDummy, CUSTOM_TTL);
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        verify(globalCapabilitiesDirectoryProxyMock,
               times(1)).lookup(callbackArrayOfGlobalDiscoveryEntryCaptor.capture(),
                                eq(domainsStrArrayDummy),
                                eq(interfaceNameDummy));
        Callback<GlobalDiscoveryEntry[]> callback = callbackArrayOfGlobalDiscoveryEntryCaptor.getValue();
        return callback;
    }

    @Test
    public void testLookupDomainsOnFailure() {
        @SuppressWarnings("unchecked")
        Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock = (Callback<List<GlobalDiscoveryEntry>>) mock(Callback.class);
        Callback<GlobalDiscoveryEntry[]> callback = lookupDomainsHelper(callbackListOfGlobalDiscoveryEntriesMock);
        JoynrRuntimeException error = new JoynrRuntimeException();
        callback.onFailure(error);
        verify(callbackListOfGlobalDiscoveryEntriesMock).onFailure(eq(error));
        verify(callbackListOfGlobalDiscoveryEntriesMock, times(0)).onSuccess(anyListOf(GlobalDiscoveryEntry.class));
    }

    @Test
    public void testLookupDomainsOnSuccess() {
        @SuppressWarnings("unchecked")
        Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock = (Callback<List<GlobalDiscoveryEntry>>) mock(Callback.class);
        Callback<GlobalDiscoveryEntry[]> callback = lookupDomainsHelper(callbackListOfGlobalDiscoveryEntriesMock);
        GlobalDiscoveryEntry[] result = new GlobalDiscoveryEntry[]{ new GlobalDiscoveryEntry(),
                new GlobalDiscoveryEntry() };
        callback.onSuccess(result);
        verify(callbackListOfGlobalDiscoveryEntriesMock).onSuccess(anyListOf(GlobalDiscoveryEntry.class));
        verify(callbackListOfGlobalDiscoveryEntriesMock, times(0)).onFailure(any(JoynrRuntimeException.class));
    }

    @Test
    public void testTouch() {
        messagingQos.setTtl_ms(FRESHNESS_UPDATE_INTERVAL_MS);
        subject.touch();
        verify(capabilitiesProxyBuilderMock).setMessagingQos(eq(messagingQos));
        verify(globalCapabilitiesDirectoryProxyMock).touch(any(String.class));
    }

}
