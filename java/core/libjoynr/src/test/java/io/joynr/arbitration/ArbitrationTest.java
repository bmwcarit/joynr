package io.joynr.arbitration;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.capabilities.CapabilitiesCallback;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.CapabilityScope;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrArbitrationException;

import java.util.ArrayList;
import java.util.List;

import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderQosRequirements;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import com.google.common.collect.Lists;

public class ArbitrationTest {

    private static final long ARBITRATION_TIMEOUT = 1000;
    private String domain = "testDomain";
    private static String interfaceName = "testInterface";
    private DiscoveryQos discoveryQos;
    String testKeyword = "testKeyword";
    long testPriority = 42;

    public interface TestInterface extends JoynrInterface {
        public static final String INTERFACE_NAME = interfaceName;
    }

    @Mock
    private LocalCapabilitiesDirectory capabilitiesSource;
    @Mock
    private ArbitrationCallback arbitrationCallback;
    protected ArrayList<CapabilityEntry> capabilitiesList;
    private String expectedParticipantId = "expectedParticipantId";
    EndpointAddressBase expectedEndpointAddress;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        capabilitiesList = new ArrayList<CapabilityEntry>();

        Mockito.doAnswer(new Answer<Object>() {

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[4] instanceof CapabilitiesCallback);
                ((CapabilitiesCallback) arguments[4]).processCapabilitiesReceived(capabilitiesList);
                return null;
            }
        }).when(capabilitiesSource).getCapabilities(Mockito.eq(domain),
                                                    Mockito.eq(interfaceName),
                                                    Mockito.<ProviderQosRequirements> any(),
                                                    Mockito.<DiscoveryQos> any(),
                                                    Mockito.<CapabilitiesCallback> any());

    }

    @Test
    public void keywordArbitratorTest() {
        ProviderQos providerQos = new ProviderQos();
        List<CustomParameter> qosParamterList = Lists.newArrayList();
        qosParamterList.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword));
        providerQos.setCustomParameters(qosParamterList);
        expectedEndpointAddress = new JoynrMessagingEndpointAddress("testChannelId");
        ArrayList<EndpointAddressBase> expectedEndpointAddresses = new ArrayList<EndpointAddressBase>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos,
                                                 expectedEndpointAddresses,
                                                 expectedParticipantId,
                                                 CapabilityScope.LOCALONLY));
        ProviderQos providerQos2 = new ProviderQos();
        List<CustomParameter> qosParamterList2 = Lists.newArrayList();
        qosParamterList2.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword"));
        providerQos2.setCustomParameters(qosParamterList2);

        EndpointAddressBase otherEndpointAddress = new JoynrMessagingEndpointAddress("otherChannelId");
        ArrayList<EndpointAddressBase> otherEndpointAddresses = new ArrayList<EndpointAddressBase>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos2,
                                                 otherEndpointAddresses,
                                                 "wrongParticipantId",
                                                 CapabilityScope.LOCALONLY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, capabilitiesSource);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId,
                                                                          expectedEndpointAddresses)));
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void keyWordArbitratorMissingKeywordTest() {

        ProviderQos providerQos = new ProviderQos();
        List<CustomParameter> qosParamterList = Lists.newArrayList();
        qosParamterList.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "wrongkeyword"));
        providerQos.setCustomParameters(qosParamterList);

        expectedEndpointAddress = new JoynrMessagingEndpointAddress("testChannelId");
        ArrayList<EndpointAddressBase> expectedEndpointAddresses = new ArrayList<EndpointAddressBase>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos,
                                                 expectedEndpointAddresses,
                                                 expectedParticipantId,
                                                 CapabilityScope.LOCALONLY));
        ProviderQos providerQos2 = new ProviderQos();
        List<CustomParameter> qosParamterList2 = Lists.newArrayList();
        qosParamterList2.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword"));
        providerQos2.setCustomParameters(qosParamterList2);

        EndpointAddressBase otherEndpointAddress = new JoynrMessagingEndpointAddress("otherChannelId");
        ArrayList<EndpointAddressBase> otherEndpointAddresses = new ArrayList<EndpointAddressBase>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos2,
                                                 otherEndpointAddresses,
                                                 "wrongParticipantId",
                                                 CapabilityScope.LOCALONLY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, capabilitiesSource);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationCanceledForever);
            Mockito.verify(arbitrationCallback, Mockito.never())
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId,
                                                                          expectedEndpointAddresses)));
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    // Check that the keyword arbitrator will only consider providers that support onChange subscriptions
    // when this is requested by the DiscoveryQos
    @Test
    public void keywordArbitratorOnChangeSubscriptionsTest() {
        ProviderQos providerQos = new ProviderQos();
        List<CustomParameter> qosParamterList = Lists.newArrayList();
        qosParamterList.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword));

        // Create a capability entry for a provider with the correct keyword but that does not support onChange subscriptions
        providerQos.setCustomParameters(qosParamterList);
        providerQos.setSupportsOnChangeSubscriptions(false);
        EndpointAddressBase otherEndpointAddress = new JoynrMessagingEndpointAddress("otherChannelId");
        ArrayList<EndpointAddressBase> otherEndpointAddresses = new ArrayList<EndpointAddressBase>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos,
                                                 otherEndpointAddresses,
                                                 "wrongParticipantId",
                                                 CapabilityScope.LOCALONLY));

        // Create a capability entry for a provider with the correct keyword and that also supports onChange subscriptions
        ProviderQos providerQos2 = new ProviderQos();
        List<CustomParameter> qosParamterList2 = Lists.newArrayList();
        qosParamterList2.add(new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword));
        providerQos2.setCustomParameters(qosParamterList2);
        providerQos2.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = new JoynrMessagingEndpointAddress("testChannelId");
        ArrayList<EndpointAddressBase> expectedEndpointAddresses = new ArrayList<EndpointAddressBase>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos2,
                                                 expectedEndpointAddresses,
                                                 "expectedParticipantId",
                                                 CapabilityScope.LOCALONLY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        discoveryQos.setProviderMustSupportOnChange(true);
        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, capabilitiesSource);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId,
                                                                          expectedEndpointAddresses)));
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testPriorityArbitrator() {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);

        expectedEndpointAddress = new JoynrMessagingEndpointAddress("testChannelId");
        ArrayList<EndpointAddressBase> expectedEndpointAddresses = new ArrayList<EndpointAddressBase>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos,
                                                 expectedEndpointAddresses,
                                                 expectedParticipantId,
                                                 CapabilityScope.LOCALONLY));
        long lessPrior = 1;
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(lessPrior);

        EndpointAddressBase otherEndpointAddress = new JoynrMessagingEndpointAddress("otherChannelId");
        ArrayList<EndpointAddressBase> otherEndpointAddresses = new ArrayList<EndpointAddressBase>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos2,
                                                 otherEndpointAddresses,
                                                 "wrongParticipantId",
                                                 CapabilityScope.LOCALONLY));
        long negativePriority = -10;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        EndpointAddressBase thirdEndpointAddress = new JoynrMessagingEndpointAddress("thirdChannelId");
        ArrayList<EndpointAddressBase> thirdEndpointAddresses = new ArrayList<EndpointAddressBase>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos3,
                                                 thirdEndpointAddresses,
                                                 "thirdParticipantId",
                                                 CapabilityScope.LOCALONLY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, capabilitiesSource);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId,
                                                                          expectedEndpointAddresses)));
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void testPriorityArbitratorWithOnlyNegativePriorities() {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(Long.MIN_VALUE);

        expectedEndpointAddress = new JoynrMessagingEndpointAddress("testChannelId");
        ArrayList<EndpointAddressBase> expectedEndpointAddresses = new ArrayList<EndpointAddressBase>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos,
                                                 expectedEndpointAddresses,
                                                 expectedParticipantId,
                                                 CapabilityScope.LOCALONLY));
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(Long.MIN_VALUE);

        EndpointAddressBase otherEndpointAddress = new JoynrMessagingEndpointAddress("otherChannelId");
        ArrayList<EndpointAddressBase> otherEndpointAddresses = new ArrayList<EndpointAddressBase>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos2,
                                                 otherEndpointAddresses,
                                                 "wrongParticipantId",
                                                 CapabilityScope.LOCALONLY));
        long negativePriority = Long.MIN_VALUE;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        EndpointAddressBase thirdEndpointAddress = new JoynrMessagingEndpointAddress("thirdChannelId");
        ArrayList<EndpointAddressBase> thirdEndpointAddresses = new ArrayList<EndpointAddressBase>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos3,
                                                 thirdEndpointAddresses,
                                                 "thirdParticipantId",
                                                 CapabilityScope.LOCALONLY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, capabilitiesSource);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.atLeast(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationCanceledForever);
            Mockito.verify(arbitrationCallback, Mockito.never())
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId,
                                                                          expectedEndpointAddresses)));
        } catch (JoynrArbitrationException e) {
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

        expectedEndpointAddress = new JoynrMessagingEndpointAddress("testChannelId");
        ArrayList<EndpointAddressBase> expectedEndpointAddresses = new ArrayList<EndpointAddressBase>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos,
                                                 expectedEndpointAddresses,
                                                 expectedParticipantId,
                                                 CapabilityScope.LOCALONLY));

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(testPriority + 1);
        providerQos2.setSupportsOnChangeSubscriptions(false);

        EndpointAddressBase otherEndpointAddress = new JoynrMessagingEndpointAddress("otherChannelId");
        ArrayList<EndpointAddressBase> otherEndpointAddresses = new ArrayList<EndpointAddressBase>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos2,
                                                 otherEndpointAddresses,
                                                 "wrongParticipantId",
                                                 CapabilityScope.LOCALONLY));

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(testPriority + 2);
        providerQos3.setSupportsOnChangeSubscriptions(false);

        EndpointAddressBase thirdEndpointAddress = new JoynrMessagingEndpointAddress("thirdChannelId");
        ArrayList<EndpointAddressBase> thirdEndpointAddresses = new ArrayList<EndpointAddressBase>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        capabilitiesList.add(new CapabilityEntry(domain,
                                                 TestInterface.class,
                                                 providerQos3,
                                                 thirdEndpointAddresses,
                                                 "thirdParticipantId",
                                                 CapabilityScope.LOCALONLY));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        discoveryQos.setProviderMustSupportOnChange(true);

        try {
            Arbitrator arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, capabilitiesSource);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.startArbitration();
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .notifyArbitrationStatusChanged(ArbitrationStatus.ArbitrationRunning);
            Mockito.verify(arbitrationCallback, Mockito.times(1))
                   .setArbitrationResult(Mockito.eq(ArbitrationStatus.ArbitrationSuccesful),
                                         Mockito.eq(new ArbitrationResult(expectedParticipantId,
                                                                          expectedEndpointAddresses)));
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
            Assert.fail("A Joyn Arbitration Exception has been thrown");
        }
    }
}
