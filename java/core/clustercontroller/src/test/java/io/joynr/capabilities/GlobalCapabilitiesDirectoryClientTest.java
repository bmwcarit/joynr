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
import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyList;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.List;
import java.util.Properties;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.util.StringArrayMatcher;
import joynr.Message;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

@RunWith(MockitoJUnitRunner.class)
public class GlobalCapabilitiesDirectoryClientTest {
    private static final long FRESHNESS_UPDATE_INTERVAL_MS = 42;
    private static final String GBID_DEFAULT_BACKEND = "joynrbackend1";
    private static final String GBID_OTHER_BACKEND = "joynrbackend2";
    private static final String[] GBIDS_ARRAY_PROPERTY_SETTING = { GBID_DEFAULT_BACKEND, GBID_OTHER_BACKEND };

    @Mock
    private ProxyBuilder<GlobalCapabilitiesDirectoryProxy> capabilitiesProxyBuilderMock;

    @Mock
    private GlobalCapabilitiesDirectoryProxy globalCapabilitiesDirectoryProxyMock;

    @Mock
    private Callback<Void> callbackVoidMock;

    @Mock
    private CallbackWithModeledError<Void, DiscoveryError> addCallbackWithModeledErrorMock;

    @Mock
    private CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError> lookupParticipantCallbackWithModeledErrorMock;

    @Mock
    private CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError> lookupDomainCallbackWithModeledErrorMock;

    @Mock
    GlobalDiscoveryEntry capabilitiesDirectoryEntryMock;

    @Mock
    private ProxyBuilderFactory proxyBuilderFactoryMock;

    private GlobalCapabilitiesDirectoryClient subject;

    private final MessagingQos expectedGcdCallMessagingQos = new MessagingQos();

    private String channelId = "GlobalCapabilitiesDirectoryClientTest_" + createUuidString();

    @Before
    public void setup() {
        final String domainMock = "domainMock";
        when(capabilitiesDirectoryEntryMock.getDomain()).thenReturn(domainMock);

        Properties properties = new Properties();
        properties.put(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS,
                       String.valueOf(FRESHNESS_UPDATE_INTERVAL_MS));
        properties.put(MessagingPropertyKeys.CHANNELID, channelId);

        subject = createGCDClientWithProperties(properties);

        when(proxyBuilderFactoryMock.get(domainMock,
                                         GlobalCapabilitiesDirectoryProxy.class)).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setDiscoveryQos(any(DiscoveryQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        lenient().when(capabilitiesProxyBuilderMock.setMessagingQos(any(MessagingQos.class)))
                 .thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.build()).thenReturn(globalCapabilitiesDirectoryProxyMock);

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
        String[] gbids = new String[]{ GBID_DEFAULT_BACKEND, GBID_OTHER_BACKEND };
        // given some discovery entry
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);
        int customTTL = 120000;
        expectedGcdCallMessagingQos.setTtl_ms(customTTL);

        // when we call the add method with it
        subject.add(addCallbackWithModeledErrorMock, capabilitiesDirectoryEntryMock, customTTL, gbids);

        // then the GCD proxy is called with the expected parameters and QoS
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(addCallbackWithModeledErrorMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         argThat(new StringArrayMatcher(gbids)),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testAddWithNonDefaultSingleGbid() {
        // given a desired gbid and some global discovery entry
        final String targetGbid = "myjoynrbackend";
        final String[] gbids = new String[]{ targetGbid };
        final GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);
        int customTTL = 120000;
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        expectedGcdCallMessagingQos.setTtl_ms(customTTL);

        // when we call the add method with them
        subject.add(addCallbackWithModeledErrorMock, capabilitiesDirectoryEntryMock, customTTL, gbids);

        // then the custom header in the GCD proxy call contains the desired gbid
        // and the call as well gets the desired callback and global discovery entry
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(addCallbackWithModeledErrorMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         argThat(new StringArrayMatcher(gbids)),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveSingleParticipantFromMultipleGbids() {
        // given some participantId and GBIDs
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{ "myjoynrbackend1", "myjoynrbackend2" };
        // when we call the remove method with it
        subject.remove(addCallbackWithModeledErrorMock, testParticipantId, targetGbids);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(addCallbackWithModeledErrorMock),
                                                            eq(testParticipantId),
                                                            eq(targetGbids.clone()),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveSingleParticipantFromSingleGbid() {
        // given some participantId and GBIDs
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{ "myjoynrbackend" };
        // when we call the remove method with it
        subject.remove(addCallbackWithModeledErrorMock, testParticipantId, targetGbids);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(addCallbackWithModeledErrorMock),
                                                            eq(testParticipantId),
                                                            eq(targetGbids),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test(expected = IllegalStateException.class)
    public void testFailOnRemoveWithoutGbids() {
        // given some participantId
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{};

        // when we call the remove method with it
        subject.remove(addCallbackWithModeledErrorMock, testParticipantId, targetGbids);
    }

    @Test
    public void testLookupParticipantId() {
        // given desired gbids, some participantId and a callback
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{ "myjoynrbackendForGCDcomm", "myjoynrbackend2" };
        final String[] expectedGbids = targetGbids.clone();
        final int customTTL = 120000;

        // when we call the lookup method with them as well as with a custom ttl
        subject.lookup(lookupParticipantCallbackWithModeledErrorMock, testParticipantId, customTTL, targetGbids);

        // then the custom header in the GCD proxy call contains the desired gbid
        // and the call as well gets the desired callback and participantId
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, expectedGbids[0]);
        expectedGcdCallMessagingQos.setTtl_ms(customTTL);
        verify(globalCapabilitiesDirectoryProxyMock).lookup(eq(lookupParticipantCallbackWithModeledErrorMock),
                                                            eq(testParticipantId),
                                                            eq(expectedGbids),
                                                            eq(expectedGcdCallMessagingQos));
    }

    private void checkLookupDomainsIsCalledCorrectly() {
        // given desired gbids, some callback (the method parameter)...
        // ...and an interface plus an array of domains
        final String[] targetGbids = new String[]{ "myjoynrbackendForGCDcomm", "myjoynrbackend2" };
        final String[] expectedGbids = targetGbids.clone();
        String[] domainsStrArrayDummy = new String[]{ "dummyDomain1", "dummyDomain2", "dummyDomain3" };
        String interfaceNameDummy = "interfaceNameDummy";
        final int customTTL = 120000;

        // when we call this GCD client with them as well as with custom ttl
        subject.lookup(lookupDomainCallbackWithModeledErrorMock,
                       domainsStrArrayDummy,
                       interfaceNameDummy,
                       customTTL,
                       targetGbids);

        // then the GCD proxy is called with the expected parameters
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, expectedGbids[0]);
        expectedGcdCallMessagingQos.setTtl_ms(customTTL);
        verify(globalCapabilitiesDirectoryProxyMock).lookup(Mockito.<CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError>> any(),
                                                            eq(domainsStrArrayDummy),
                                                            eq(interfaceNameDummy),
                                                            eq(expectedGbids),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testLookupDomainsOnDiscoveryError() {
        // given the GCD proxy returns an error on lookup with domain/interface...
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError> callback = (CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError>) invocation.getArguments()[0];
                callback.onFailure(DiscoveryError.INTERNAL_ERROR);
                return null;
            }
        }).when(globalCapabilitiesDirectoryProxyMock)
          .lookup(any(), any(String[].class), any(String.class), any(String[].class), any(MessagingQos.class));
        // ...and given some callback

        // when we execute the lookup method of the GCD client with this callback
        checkLookupDomainsIsCalledCorrectly();

        // then the callback we used for the GCD client call is correctly completed
        verify(lookupDomainCallbackWithModeledErrorMock).onFailure(DiscoveryError.INTERNAL_ERROR);
        verify(lookupDomainCallbackWithModeledErrorMock, times(0)).onSuccess(anyList());
    }

    @Test
    public void testLookupDomainsOnJoynrRuntimeException() {
        // given the GCD proxy returns an error on lookup with domain/interface...
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError> callback = (CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError>) invocation.getArguments()[0];
                JoynrRuntimeException error = new JoynrRuntimeException();
                callback.onFailure(error);
                return null;
            }
        }).when(globalCapabilitiesDirectoryProxyMock)
          .lookup(any(), any(String[].class), any(String.class), any(String[].class), any(MessagingQos.class));
        // ...and given some callback

        // when we execute the lookup method of the GCD client with this callback
        checkLookupDomainsIsCalledCorrectly();

        // then the callback we used for the GCD client call is correctly completed
        verify(lookupDomainCallbackWithModeledErrorMock).onFailure(eq(new JoynrRuntimeException()));
        verify(lookupDomainCallbackWithModeledErrorMock, times(0)).onSuccess(anyList());
    }

    @Test
    public void testLookupDomainsOnSuccess() {
        // given the GCD proxy returns a regular answer on lookup with domain/interface...
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError> callback = (CallbackWithModeledError<GlobalDiscoveryEntry[], DiscoveryError>) invocation.getArguments()[0];
                GlobalDiscoveryEntry[] result = new GlobalDiscoveryEntry[]{ new GlobalDiscoveryEntry(),
                        new GlobalDiscoveryEntry() };
                callback.onSuccess(result);
                return null;
            }
        }).when(globalCapabilitiesDirectoryProxyMock)
          .lookup(any(), any(String[].class), any(String.class), any(String[].class), any(MessagingQos.class));
        // ...and given some callback

        // when we execute the lookup method of the GCD client with this callback
        checkLookupDomainsIsCalledCorrectly();

        // then the callback we used for the GCD client call is correctly completed
        verify(lookupDomainCallbackWithModeledErrorMock).onSuccess(anyList());
        verify(lookupDomainCallbackWithModeledErrorMock, times(0)).onFailure(any(JoynrRuntimeException.class));
    }

    @Test
    public void testRemoveStale() {
        // Test whether removeStale() of the GlobalCapabilitiesDirectoryProxy 
        // called once with the given gbid and value of max last seen date in milliseconds.
        final long maxLastSeenDateMs = 1000000L;
        final long expectedRemoveStaleTtl = 60 * 1000L;
        final MessagingQos messagingQos = new MessagingQos(expectedRemoveStaleTtl);
        messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, GBID_DEFAULT_BACKEND);

        subject.removeStale(callbackVoidMock, maxLastSeenDateMs, GBID_DEFAULT_BACKEND);

        verify(globalCapabilitiesDirectoryProxyMock, times(1)).removeStale(eq(callbackVoidMock),
                                                                           eq(channelId),
                                                                           eq(maxLastSeenDateMs),
                                                                           eq(messagingQos));
    }

    @Test
    public void testTouchAsync() {
        final String[] testParticipantIds = new String[]{ "participantId1", "participantId2" };
        final String[] expectedParticipantIds = testParticipantIds.clone();
        final String expectedGbid = "dummyGbid";

        subject.touch(callbackVoidMock, testParticipantIds, expectedGbid);

        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        verify(globalCapabilitiesDirectoryProxyMock, times(1)).touch(eq(callbackVoidMock),
                                                                     eq(channelId),
                                                                     eq(expectedParticipantIds),
                                                                     messagingQosCaptor.capture());

        assertEquals(FRESHNESS_UPDATE_INTERVAL_MS, messagingQosCaptor.getValue().getRoundTripTtl_ms());
        assertTrue(messagingQosCaptor.getValue().getCustomMessageHeaders().containsKey(Message.CUSTOM_HEADER_GBID_KEY));
        assertEquals(expectedGbid,
                     messagingQosCaptor.getValue().getCustomMessageHeaders().get(Message.CUSTOM_HEADER_GBID_KEY));
    }
}
