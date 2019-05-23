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
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyListOf;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.stream.Collectors;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

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
import joynr.Message;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.GlobalDiscoveryEntry;

@RunWith(MockitoJUnitRunner.class)
public class GlobalCapabilitiesDirectoryClientTest {
    private static final long DEFAULT_TTL_ADD_AND_REMOVE = 60L * 1000L;
    private static final long CUSTOM_TTL = 3L * 1000L;
    private static final long FRESHNESS_UPDATE_INTERVAL_MS = 42;
    private static final String GBID_DEFAULT_BACKEND = "joynrbackend1";
    private static final String[] GBIDS_ARRAY_PROPERTY_SETTING = { GBID_DEFAULT_BACKEND, "joynrbackend2" };

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

    private final MessagingQos messagingQos = new MessagingQos();
    private final MessagingQos expectedGcdCallMessagingQos = new MessagingQos();

    @Before
    public void setup() {
        final String domainMock = "domainMock";
        when(capabilitiesDirectoryEntryMock.getDomain()).thenReturn(domainMock);

        Properties properties = new Properties();
        properties.put(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS,
                       String.valueOf(FRESHNESS_UPDATE_INTERVAL_MS));

        subject = createGCDClientWithProperties(properties);

        when(proxyBuilderFactoryMock.get(domainMock,
                                         GlobalCapabilitiesDirectoryProxy.class)).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setDiscoveryQos(any(DiscoveryQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setMessagingQos(any(MessagingQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.build()).thenReturn(globalCapabilitiesDirectoryProxyMock);

        // expect default ttl if not changed in the test case
        expectedGcdCallMessagingQos.setTtl_ms(DEFAULT_TTL_ADD_AND_REMOVE);
        // expect the default backend in the custom header if not changed in the test case
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, GBID_DEFAULT_BACKEND);
    }

    private GlobalCapabilitiesDirectoryClient createGCDClientWithProperties(final Properties properties) {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            public void configure() {
                bind(ProxyBuilderFactory.class).toInstance(proxyBuilderFactoryMock);
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntryMock);
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                    .toInstance(GBIDS_ARRAY_PROPERTY_SETTING);
            }
        }, new JoynrPropertiesModule(properties));

        return injector.getInstance(GlobalCapabilitiesDirectoryClient.class);
    }

    @Test
    public void testAdd() {
        // given some discovery entry
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);

        // when we call the add method with it
        subject.add(callbackMock, capabilitiesDirectoryEntryMock);

        // then the GCD proxy is called with the expected parameters and QoS
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testAddWithCustomTTL() {
        // given a GCD client with custom ttl...
        Properties properties = new Properties();
        properties.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS,
                       String.valueOf(CUSTOM_TTL));
        GlobalCapabilitiesDirectoryClient subjectInject = createGCDClientWithProperties(properties);
        // ...and some discovery entry
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);

        // when we call the add method on this client
        subjectInject.add(callbackMock, capabilitiesDirectoryEntryMock);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testAddWithGBID() {
        // given a desired gbid and some global discovery entry
        final String targetGbid = "myjoynrbackend";
        final GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);

        // when we call the add method with them
        subject.add(callbackMock, capabilitiesDirectoryEntryMock, targetGbid);

        // then the custom header in the GCD proxy call contains the desired gbid
        // and the call as well gets the desired callback and global discovery entry
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveSingleParticipant() {
        // given some participantId
        final String testParticipantId = "testParticipantId";

        // when we call the remove method with it
        subject.remove(callbackMock, testParticipantId);

        // then the GCD proxy is called with the expected parameters and QoS
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock),
                                                            eq(testParticipantId),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveSingleParticipantWithGbid() {
        // given some participantId and a desired gbid
        final String testParticipantId = "testParticipantId";
        final String targetGbid = "myjoynrbackend";

        // when we call the remove method with them
        subject.remove(callbackMock, testParticipantId, targetGbid);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock),
                                                            eq(testParticipantId),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveParticipantsList() {
        // given some participantId list
        List<String> testParticipantIdList = Arrays.asList("testParticipantId", "testParticipantId2");

        // when we call the remove method with them
        subject.remove(callbackMock, testParticipantIdList);

        // then the GCD proxy is called with the expected parameters and QoS
        String[] testParticipantIdToArray = testParticipantIdList.stream().toArray(String[]::new);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock),
                                                            eq(testParticipantIdToArray),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveParticipantsListWithGbid() {
        // given a desired gbid and some participantId list
        final String targetGbid = "myjoynrbackend";
        final List<String> testParticipantIdList = Arrays.asList("testParticipantId", "testParticipantId2");

        // when we call the remove method with them
        subject.remove(callbackMock, testParticipantIdList, targetGbid);

        // then the custom header in the GCD proxy call contains the desired gbid
        // and the call as well gets the desired callback and participantId array
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        String[] expectedParticipantIdArray = testParticipantIdList.stream().toArray(String[]::new);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock),
                                                            eq(expectedParticipantIdArray),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveParticipantsListWithCustomTTL() {
        // given a GCD client with custom ttl...
        Properties properties = new Properties();
        properties.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS,
                       String.valueOf(CUSTOM_TTL));
        GlobalCapabilitiesDirectoryClient subjectInject = createGCDClientWithProperties(properties);
        // ...and some participantId list
        List<String> testParticipantIdList = Arrays.asList("testParticipantId", "testParticipantId2");

        // when we call the remove method on this client
        subjectInject.remove(callbackMock, testParticipantIdList);

        // then the GCD proxy is called with the expected parameters and QoS
        String[] testParticipantIdToArray = testParticipantIdList.stream().toArray(String[]::new);
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackMock),
                                                            eq(testParticipantIdToArray),
                                                            eq(expectedGcdCallMessagingQos));
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
