package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.proxy.Callback;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class ArbitrationTest {

    private static final long ARBITRATION_TIMEOUT = 1000;
    private static final Long NO_EXPIRY = Long.MAX_VALUE;
    private String domain = "testDomain";
    private static String interfaceName = "testInterface";
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
    protected ArrayList<DiscoveryEntry> capabilitiesList;
    private String expectedParticipantId = "expectedParticipantId";
    Address expectedEndpointAddress;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        capabilitiesList = new ArrayList<DiscoveryEntry>();

        Mockito.doAnswer(new Answer<Object>() {

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[0] instanceof Callback);
                ((Callback) arguments[0]).resolve((Object) capabilitiesList.toArray(new DiscoveryEntry[0]));
                return null;
            }
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback> any(),
                                                 Mockito.eq(domain),
                                                 Mockito.eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

    }

    @Test
    public void keywordArbitratorTest() {
        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos.setCustomParameters(qosParameters);
        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                expectedParticipantId,
                                                providerQos,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword") };
        providerQos2.setCustomParameters(qosParameters2);

        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "wrongParticipantId",
                                                providerQos2,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain,
                                                             interfaceName,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId)));
        } catch (DiscoveryException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void keyWordArbitratorMissingKeywordTest() {

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "wrongkeyword") };
        providerQos.setCustomParameters(qosParameters);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                expectedParticipantId,
                                                providerQos,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword") };
        providerQos2.setCustomParameters(qosParameters2);

        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "wrongParticipantId",
                                                providerQos2,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        int discoveryTimeout = 0; // use minimal timeout to prevent restarting arbitration
        discoveryQos = new DiscoveryQos(discoveryTimeout, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain,
                                                             interfaceName,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationCanceledForever);
            Mockito.verify(arbitrationCallback, Mockito.never())
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId)));
        } catch (DiscoveryException e) {
            e.printStackTrace();
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
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "wrongParticipantId",
                                                providerQos,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        // Create a capability entry for a provider with the correct keyword and that also supports onChange subscriptions
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos2.setCustomParameters(qosParameters2);
        providerQos2.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "expectedParticipantId",
                                                providerQos2,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        discoveryQos.setProviderMustSupportOnChange(true);
        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain,
                                                             interfaceName,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId)));
        } catch (DiscoveryException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testPriorityArbitrator() {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                expectedParticipantId,
                                                providerQos,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));
        long lessPrior = 1;
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(lessPrior);

        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "wrongParticipantId",
                                                providerQos2,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));
        long negativePriority = -10;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        Address thirdEndpointAddress = new ChannelAddress("http://testUrl", "thirdChannelId");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "thirdParticipantId",
                                                providerQos3,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain,
                                                             interfaceName,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId)));
        } catch (DiscoveryException e) {
            e.printStackTrace();
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
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                expectedParticipantId,
                                                providerQos,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(Long.MIN_VALUE);

        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "wrongParticipantId",
                                                providerQos2,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));
        long negativePriority = Long.MIN_VALUE;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "thirdParticipantId",
                                                providerQos3,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain,
                                                             interfaceName,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.atLeast(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationCanceledForever);
            Mockito.verify(arbitrationCallback, Mockito.never())
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId)));
        } catch (DiscoveryException e) {
            e.printStackTrace();
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
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                expectedParticipantId,
                                                providerQos,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(testPriority + 1);
        providerQos2.setSupportsOnChangeSubscriptions(false);

        Address otherEndpointAddress = new ChannelAddress("http://testUrl", "otherChannelId");
        ArrayList<Address> otherEndpointAddresses = new ArrayList<Address>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "wrongParticipantId",
                                                providerQos2,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(testPriority + 2);
        providerQos3.setSupportsOnChangeSubscriptions(false);

        Address thirdEndpointAddress = new ChannelAddress("http://testUrl", "thirdChannelId");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new DiscoveryEntry(new Version(47, 11),
                                                domain,
                                                TestInterface.INTERFACE_NAME,
                                                "thirdParticipantId",
                                                providerQos3,
                                                System.currentTimeMillis(),
                                                NO_EXPIRY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        discoveryQos.setProviderMustSupportOnChange(true);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain,
                                                             interfaceName,
                                                             discoveryQos,
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId)));
        } catch (DiscoveryException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testCustomArbitrationFunction() {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain,
                                                           TestInterface.INTERFACE_NAME,
                                                           expectedParticipantId,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           NO_EXPIRY);
        capabilitiesList.add(discoveryEntry);

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(any(Map.class), any(Collection.class))).thenReturn(discoveryEntry);
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.startArbitration();

        verify(arbitrationStrategyFunction, times(1)).select(eq(discoveryQos.getCustomParameters()),
                                                             eq(capabilitiesList));
        verify(arbitrationCallback, times(1)).notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
        verify(arbitrationCallback, times(1)).setArbitrationResult(eq(ArbitrationStatus.ArbitrationSuccesful),
                                                                   eq(new ArbitrationResult(expectedParticipantId)));

    }
}
