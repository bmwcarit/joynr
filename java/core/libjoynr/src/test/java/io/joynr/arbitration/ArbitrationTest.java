package io.joynr.arbitration;

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

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.proxy.Callback;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class ArbitrationTest {

    private static final long ARBITRATION_TIMEOUT = 1000;
    private static final Long NO_EXPIRY = Long.MAX_VALUE;
    private String domain = "testDomain";
    private String publicKeyId = "publicKeyId";
    private static String interfaceName = "testInterface";
    private static Version interfaceVersion = new Version(0, 0);
    private DiscoveryQos discoveryQos;
    String testKeyword = "testKeyword";
    long testPriority = 42;

    public interface TestInterface {
        public static final String INTERFACE_NAME = interfaceName;
    }

    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    @Mock
    private ArbitrationCallback arbitrationCallback;
    @Mock
    private DiscoveryEntryVersionFilter discoveryEntryVersionFilter;
    protected ArrayList<DiscoveryEntryWithMetaInfo> capabilitiesList;
    private String expectedParticipantId = "expectedParticipantId";
    Address expectedEndpointAddress;

    @SuppressWarnings({ "unchecked", "rawtypes" })
    @Before
    public void setUp() throws Exception {
        initMocks(this);

        capabilitiesList = new ArrayList<DiscoveryEntryWithMetaInfo>();

        doAnswer(new Answer<Object>() {

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[0] instanceof Callback);
                ((Callback) arguments[0]).resolve((Object) capabilitiesList.toArray(new DiscoveryEntryWithMetaInfo[0]));
                return null;
            }
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback> any(),
                                                 Mockito.eq(new String[]{ domain }),
                                                 Mockito.eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

        Field discoveryEntryVersionFilterField = ArbitratorFactory.class.getDeclaredField("discoveryEntryVersionFilter");
        discoveryEntryVersionFilterField.setAccessible(true);
        discoveryEntryVersionFilterField.set(ArbitratorFactory.class, discoveryEntryVersionFilter);

        doAnswer(new Answer<Set<DiscoveryEntry>>() {
            @Override
            public Set<DiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                return (Set<DiscoveryEntry>) invocation.getArguments()[1];
            }
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
                                                    Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                                                    Mockito.<Map<String, Set<Version>>> any());
    }

    @Test
    public void keywordArbitratorTest() {
        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos.setCustomParameters(qosParameters);
        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword") };
        providerQos2.setCustomParameters(qosParameters2);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();

            ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onSuccess(Mockito.eq(expectedArbitrationResult));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void keyWordArbitratorMissingKeywordTest() {

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "wrongkeyword") };
        providerQos.setCustomParameters(qosParameters);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            expectedParticipantId,
                                                            providerQos,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword") };
        providerQos2.setCustomParameters(qosParameters2);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        int discoveryTimeout = 0; // use minimal timeout to prevent restarting arbitration
        discoveryQos = new DiscoveryQos(discoveryTimeout, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onError(any(Throwable.class));
            Mockito.verify(arbitrationCallback, Mockito.never()).onSuccess(Mockito.any(ArbitrationResult.class));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    // Check that the keyword arbitrator will only consider providers that support onChange subscriptions
    // when this is requested by the DiscoveryQos
    @Test
    public void keywordArbitratorOnChangeSubscriptionsTest() {
        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };

        // Create a capability entry for a provider with the correct keyword but that does not support onChange subscriptions
        providerQos.setCustomParameters(qosParameters);
        providerQos.setSupportsOnChangeSubscriptions(false);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        // Create a capability entry for a provider with the correct keyword and that also supports onChange subscriptions
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos2.setCustomParameters(qosParameters2);
        providerQos2.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           "expectedParticipantId",
                                                                                           providerQos2,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        discoveryQos.setProviderMustSupportOnChange(true);
        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();

            ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onSuccess(Mockito.eq(expectedArbitrationResult));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testLastSeenArbitrator() {
        ProviderQos providerQos = new ProviderQos();

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos,
                                                            222L,
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           333L,
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "thirdParticipantId",
                                                            providerQos,
                                                            111L,
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.LastSeen, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();

            ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onSuccess(Mockito.eq(expectedArbitrationResult));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testPriorityArbitrator() {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        long lessPrior = 1;
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(lessPrior);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        long negativePriority = -10;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        Address thirdEndpointAddress = new ChannelAddress("http://testUrl", "thirdChannelId");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "thirdParticipantId",
                                                            providerQos3,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();

            ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onSuccess(Mockito.eq(expectedArbitrationResult));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testPriorityArbitratorWithOnlyNegativePriorities() {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(Long.MIN_VALUE);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        ArrayList<Address> expectedEndpointAddresses = new ArrayList<Address>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            expectedParticipantId,
                                                            providerQos,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(Long.MIN_VALUE);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        long negativePriority = Long.MIN_VALUE;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "thirdParticipantId",
                                                            providerQos3,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onError(any(Throwable.class));
            Mockito.verify(arbitrationCallback, Mockito.never()).onSuccess(Mockito.any(ArbitrationResult.class));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testPriorityArbitratorOnChangeSubscriptions() {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);
        providerQos.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(testPriority + 1);
        providerQos2.setSupportsOnChangeSubscriptions(false);

        Address otherEndpointAddress = new ChannelAddress("http://testUrl", "otherChannelId");
        ArrayList<Address> otherEndpointAddresses = new ArrayList<Address>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(testPriority + 2);
        providerQos3.setSupportsOnChangeSubscriptions(false);

        Address thirdEndpointAddress = new ChannelAddress("http://testUrl", "thirdChannelId");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "thirdParticipantId",
                                                            providerQos3,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        discoveryQos.setProviderMustSupportOnChange(true);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();

            ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
            Mockito.verify(arbitrationCallback, Mockito.times(1)).onSuccess(Mockito.eq(expectedArbitrationResult));
        } catch (DiscoveryException e) {
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCustomArbitrationFunction() {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                   domain,
                                                                                   TestInterface.INTERFACE_NAME,
                                                                                   expectedParticipantId,
                                                                                   providerQos,
                                                                                   System.currentTimeMillis(),
                                                                                   NO_EXPIRY,
                                                                                   publicKeyId,
                                                                                   true);
        capabilitiesList.add(discoveryEntry);

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(any(Map.class), any(Collection.class))).thenReturn(Sets.newHashSet(discoveryEntry));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.startArbitration();

        verify(arbitrationStrategyFunction, times(1)).select(eq(discoveryQos.getCustomParameters()),
                                                             eq(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList)));

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(discoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));

    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCustomArbitrationFunctionMultipleMatches() {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        String publicKeyId = "publicKeyId";

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        String[] participantIds = new String[]{ "first-participant", "second-participant" };
        for (int index = 0; index < participantIds.length; index++) {
            DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                       domain,
                                                                                       TestInterface.INTERFACE_NAME,
                                                                                       participantIds[index],
                                                                                       providerQos,
                                                                                       System.currentTimeMillis(),
                                                                                       NO_EXPIRY,
                                                                                       publicKeyId,
                                                                                       true);
            capabilitiesList.add(discoveryEntry);
        }

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(any(Map.class), any(Collection.class))).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.startArbitration();

        verify(arbitrationStrategyFunction, times(1)).select(eq(discoveryQos.getCustomParameters()),
                                                             eq(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList)));

        DiscoveryEntryWithMetaInfo[] expectedDiscoveryEntries = new DiscoveryEntryWithMetaInfo[capabilitiesList.size()];
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(capabilitiesList.toArray(expectedDiscoveryEntries));
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testVersionFilterUsed() {
        ProviderQos providerQos = new ProviderQos();
        String publicKeyId = "publicKeyId";
        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        String[] participantIds = new String[]{ "first-participant", "second-participant" };
        for (int index = 0; index < participantIds.length; index++) {
            DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(index, index),
                                                                                       domain,
                                                                                       TestInterface.INTERFACE_NAME,
                                                                                       participantIds[index],
                                                                                       providerQos,
                                                                                       System.currentTimeMillis(),
                                                                                       NO_EXPIRY,
                                                                                       publicKeyId,
                                                                                       true);
            capabilitiesList.add(discoveryEntry);
        }

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(any(Map.class), any(Collection.class))).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.startArbitration();

        verify(discoveryEntryVersionFilter).filter(interfaceVersion,
                                                   new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList),
                                                   new HashMap<String, Set<Version>>());
    }

    @Test
    public void testIncompatibleVersionsReported() {
        Version incompatibleVersion = new Version(100, 100);
        final Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = Lists.newArrayList(new DiscoveryEntryWithMetaInfo(incompatibleVersion,
                                                                                                                          domain,
                                                                                                                          interfaceName,
                                                                                                                          "first-participant",
                                                                                                                          new ProviderQos(),
                                                                                                                          System.currentTimeMillis(),
                                                                                                                          NO_EXPIRY,
                                                                                                                          "public-key-1",
                                                                                                                          true));
        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Mockito.<Map<String, String>> any(),
                                                Mockito.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>());
        doAnswer(new Answer<Set<DiscoveryEntryWithMetaInfo>>() {
            @SuppressWarnings("unchecked")
            @Override
            public Set<DiscoveryEntryWithMetaInfo> answer(InvocationOnMock invocation) throws Throwable {
                Map<String, Set<Version>> filteredVersions = (Map<String, Set<Version>>) invocation.getArguments()[2];
                Set<DiscoveryEntryWithMetaInfo> discoveryEntries = (Set<DiscoveryEntryWithMetaInfo>) invocation.getArguments()[1];
                filteredVersions.put(domain, Sets.newHashSet(discoveryEntries.iterator().next().getProviderVersion()));
                discoveryEntries.clear();
                return new HashSet<>();
            }
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
                                                    Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                                                    Mockito.<Map<String, Set<Version>>> any());
        DiscoveryQos discoveryQos = new DiscoveryQos(10L, arbitrationStrategyFunction, 0L);
        reset(localDiscoveryAggregator);
        doAnswer(new Answer<Object>() {

            @SuppressWarnings("unchecked")
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                ((Callback<DiscoveryEntryWithMetaInfo[]>) invocation.getArguments()[0]).resolve((Object) discoveryEntries.toArray(new DiscoveryEntryWithMetaInfo[1]));
                return null;
            }
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback<DiscoveryEntryWithMetaInfo[]>> any(),
                                                 eq(new String[]{ domain }),
                                                 eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

        Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain),
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.startArbitration();

        Set<Version> discoveredVersions = Sets.newHashSet(incompatibleVersion);
        ArgumentCaptor<NoCompatibleProviderFoundException> noCompatibleProviderFoundExceptionCaptor = ArgumentCaptor.forClass(NoCompatibleProviderFoundException.class);
        verify(arbitrationCallback).onError(noCompatibleProviderFoundExceptionCaptor.capture());
        assertEquals(discoveredVersions, noCompatibleProviderFoundExceptionCaptor.getValue().getDiscoveredVersions());

    }

    @Test
    public void testMultiDomainIncompatibleVersionsReported() {
        final Version incompatibleVersion = new Version(100, 100);
        final String domain1 = "domain1";
        final String domain2 = "domain2";
        final DiscoveryEntryWithMetaInfo discoveryEntry1 = new DiscoveryEntryWithMetaInfo(incompatibleVersion,
                                                                                          domain1,
                                                                                          interfaceName,
                                                                                          "participant1",
                                                                                          new ProviderQos(),
                                                                                          System.currentTimeMillis(),
                                                                                          NO_EXPIRY,
                                                                                          "public-key-1",
                                                                                          true);
        final DiscoveryEntryWithMetaInfo discoveryEntry2 = new DiscoveryEntryWithMetaInfo(incompatibleVersion,
                                                                                          domain2,
                                                                                          interfaceName,
                                                                                          "participant2",
                                                                                          new ProviderQos(),
                                                                                          System.currentTimeMillis(),
                                                                                          NO_EXPIRY,
                                                                                          "public-key-2",
                                                                                          true);
        final Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = Lists.newArrayList(discoveryEntry1,
                                                                                           discoveryEntry2);

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Mockito.<Map<String, String>> any(),
                                                Mockito.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>());
        doAnswer(new Answer<Set<DiscoveryEntry>>() {
            @SuppressWarnings("unchecked")
            @Override
            public Set<DiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                Set<DiscoveryEntry> discoveryEntries = (Set<DiscoveryEntry>) invocation.getArguments()[1];
                Map<String, Set<Version>> filteredVersions = (Map<String, Set<Version>>) invocation.getArguments()[2];
                filteredVersions.put(domain1, Sets.newHashSet(discoveryEntry1.getProviderVersion()));
                filteredVersions.put(domain2, Sets.newHashSet(discoveryEntry2.getProviderVersion()));
                discoveryEntries.clear();
                return new HashSet<>();
            }
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
                                                    Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                                                    Mockito.<Map<String, Set<Version>>> any());
        DiscoveryQos discoveryQos = new DiscoveryQos(10L, arbitrationStrategyFunction, 0L);
        reset(localDiscoveryAggregator);
        doAnswer(new Answer<Object>() {

            @SuppressWarnings("unchecked")
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                ((Callback<DiscoveryEntryWithMetaInfo[]>) invocation.getArguments()[0]).resolve((Object) discoveryEntries.toArray(new DiscoveryEntryWithMetaInfo[2]));
                return null;
            }
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback<DiscoveryEntryWithMetaInfo[]>> any(),
                                                 any(String[].class),
                                                 eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

        Arbitrator arbitrator = ArbitratorFactory.create(Sets.newHashSet(domain1, domain2),
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.startArbitration();

        Set<Version> discoveredVersions = Sets.newHashSet(incompatibleVersion);
        ArgumentCaptor<MultiDomainNoCompatibleProviderFoundException> noCompatibleProviderFoundExceptionCaptor = ArgumentCaptor.forClass(MultiDomainNoCompatibleProviderFoundException.class);
        verify(arbitrationCallback).onError(noCompatibleProviderFoundExceptionCaptor.capture());
        assertEquals(discoveredVersions,
                     noCompatibleProviderFoundExceptionCaptor.getValue().getDiscoveredVersionsForDomain(domain2));
        assertEquals(discoveredVersions,
                     noCompatibleProviderFoundExceptionCaptor.getValue().getDiscoveredVersionsForDomain(domain1));

    }
}
