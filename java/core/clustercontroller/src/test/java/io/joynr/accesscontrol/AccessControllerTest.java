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
package io.joynr.accesscontrol;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilityCallback;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

/**
 * Test the AccessController
 */
@RunWith(MockitoJUnitRunner.class)
public class AccessControllerTest {

    private static final int ONE_MINUTE_IN_MS = 60 * 1000;

    @Mock
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;

    @Mock
    private LocalDomainAccessController localDomainAccessController;

    @Mock
    private ImmutableMessage messageMock;

    private static final String DUMMY_USERID = "dummyUserId";

    private AccessController accessController;
    private String fromParticipantId = "sender";
    private String toParticipantId = "receiver";
    private String testDomain = "testDomain";
    private String testInterface = "testInterface";
    private String testPublicKeyId = "testPublicKeyId";
    private HasConsumerPermissionCallback callback = Mockito.mock(HasConsumerPermissionCallback.class);

    @Before
    public void setup() {
        String discoveryProviderParticipantId = "";
        String routingProviderParticipantId = "";
        accessController = new AccessControllerImpl(localCapabilitiesDirectory,
                                                    localDomainAccessController,
                                                    new CapabilitiesProvisioning() {
                                                        @Override
                                                        public Collection<GlobalDiscoveryEntry> getDiscoveryEntries() {
                                                            return new ArrayList<GlobalDiscoveryEntry>();
                                                        }
                                                    },
                                                    discoveryProviderParticipantId,
                                                    routingProviderParticipantId);

        when(messageMock.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_REQUEST);
        when(messageMock.getRecipient()).thenReturn(toParticipantId);
        when(messageMock.getSender()).thenReturn(fromParticipantId);
        when(messageMock.getCreatorUserId()).thenReturn(DUMMY_USERID);
        when(messageMock.getId()).thenReturn("someId");

        final DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                         testDomain,
                                                                                         testInterface,
                                                                                         toParticipantId,
                                                                                         new ProviderQos(),
                                                                                         System.currentTimeMillis(),
                                                                                         System.currentTimeMillis()
                                                                                                 + ONE_MINUTE_IN_MS,
                                                                                         testPublicKeyId,
                                                                                         false);

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                CapabilityCallback callback = (CapabilityCallback) invocation.getArguments()[2];
                callback.processCapabilityReceived(discoveryEntry);
                return null;
            }
        }).when(localCapabilitiesDirectory)
          .lookup(eq(toParticipantId), any(DiscoveryQos.class), any(CapabilityCallback.class));

    }

    @Test
    public void testAccessWithInterfaceLevelAccessControl() {
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                GetConsumerPermissionCallback callback = (GetConsumerPermissionCallback) invocation.getArguments()[4];
                callback.getConsumerPermission(Permission.YES);
                return null;
            }

        }).when(localDomainAccessController).getConsumerPermission(eq(DUMMY_USERID),
                                                                   eq(testDomain),
                                                                   eq(testInterface),
                                                                   eq(TrustLevel.HIGH),
                                                                   any(GetConsumerPermissionCallback.class));

        accessController.hasConsumerPermission(messageMock, callback);
        verify(callback, Mockito.times(1)).hasConsumerPermission(true);
    }
}
