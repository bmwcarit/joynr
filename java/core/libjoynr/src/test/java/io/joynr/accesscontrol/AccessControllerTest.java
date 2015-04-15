package io.joynr.accesscontrol;

/*
 * #%L
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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.when;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.CapabilityEntryImpl;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.JoynrMessageFactory;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.messaging.MessagingModule;
import joynr.JoynrMessage;
import joynr.Request;
import joynr.infrastructure.Permission;
import joynr.infrastructure.TrustLevel;
import joynr.types.ProviderQos;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

/**
 * Test the AccessController
 */
@RunWith(MockitoJUnitRunner.class)
public class AccessControllerTest {

    @Mock
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;

    @Mock
    private LocalDomainAccessController localDomainAccessController;

    private static final String DUMMY_USERID = "dummyUserId";

    private static JoynrMessageFactory messageFactory;
    private JoynrMessage message;
    private static ObjectMapper objectMapper;
    private AccessController accessController;
    private Request request;
    private String fromParticipantId = "sender";
    private String toParticipantId = "receiver";
    private String testDomain = "testDomain";
    private String testInterface = "testInterface";
    private String testOperation = "testOperation";
    private ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(1000);
    private String replyToChannelId = "replyToId";

    @BeforeClass
    public static void initialize() {
        Injector injector = Guice.createInjector(new MessagingModule(), new AbstractModule() {

            @Override
            protected void configure() {
                requestStaticInjection(Request.class);

            }

        });
        objectMapper = injector.getInstance(ObjectMapper.class);
        messageFactory = injector.getInstance(JoynrMessageFactory.class);
    }

    @Before
    public void setup() {
        accessController = new AccessControllerImpl(localCapabilitiesDirectory,
                                                    localDomainAccessController,
                                                    objectMapper);

        // Create a dummy message
        request = new Request(testOperation, new String[]{}, new Class<?>[]{});
        message = messageFactory.createRequest(fromParticipantId,
                                               toParticipantId,
                                               request,
                                               expiryDate,
                                               replyToChannelId);
        message.setHeaderValue(JoynrMessage.HEADER_NAME_CREATOR_USER_ID, DUMMY_USERID);

        CapabilityEntry capabilityEntry = new CapabilityEntryImpl(testDomain,
                                                                  testInterface,
                                                                  new ProviderQos(),
                                                                  toParticipantId,
                                                                  System.currentTimeMillis(),
                                                                  new JoynrMessagingEndpointAddress("11111"));
        when(localCapabilitiesDirectory.lookup(eq(toParticipantId), any(DiscoveryQos.class))).thenReturn(capabilityEntry);
    }

    @Test
    public void testAccessWithInterfaceLevelAccessControl() {
        when(localDomainAccessController.getConsumerPermission(DUMMY_USERID, testDomain, testInterface, TrustLevel.HIGH)).thenReturn(Permission.YES);

        assertTrue(accessController.hasConsumerPermission(message));
    }

    @Test
    public void testAccessWithOperationLevelAccessControl() {
        when(localDomainAccessController.getConsumerPermission(DUMMY_USERID, testDomain, testInterface, TrustLevel.HIGH)).thenReturn(null);
        when(localDomainAccessController.getConsumerPermission(DUMMY_USERID,
                                                               testDomain,
                                                               testInterface,
                                                               testOperation,
                                                               TrustLevel.HIGH)).thenReturn(Permission.YES);

        assertTrue(accessController.hasConsumerPermission(message));
    }

    @Test
    public void testAccessWithOperationLevelAccessControlAndFaultyMessage() {
        when(localDomainAccessController.getConsumerPermission(DUMMY_USERID,
                                                               testDomain,
                                                               testInterface,
                                                               testOperation,
                                                               TrustLevel.HIGH)).thenReturn(Permission.NO);
        message.setPayload("invalid serialization of Request object");

        assertFalse(accessController.hasConsumerPermission(message));
    }
}
