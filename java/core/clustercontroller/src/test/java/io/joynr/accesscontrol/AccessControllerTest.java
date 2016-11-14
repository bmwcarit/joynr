package io.joynr.accesscontrol;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.dispatching.JoynrMessageProcessor;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingQos;
import joynr.JoynrMessage;
import joynr.Request;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderQos;
import joynr.types.Version;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

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
    private String testPublicKeyId = "testPublicKeyId";
    private MessagingQos messagingQos = new MessagingQos(1000);
    private ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());

    @BeforeClass
    public static void initialize() {
        Injector injector = Guice.createInjector(new JsonMessageSerializerModule(), new AbstractModule() {

            @Override
            protected void configure() {
                requestStaticInjection(Request.class);
                Multibinder.newSetBinder(binder(), new TypeLiteral<JoynrMessageProcessor>() {
                });
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
        message = messageFactory.createRequest(fromParticipantId, toParticipantId, request, messagingQos);
        message.setHeaderValue(JoynrMessage.HEADER_NAME_CREATOR_USER_ID, DUMMY_USERID);

        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                   testDomain,
                                                                                   testInterface,
                                                                                   toParticipantId,
                                                                                   new ProviderQos(),
                                                                                   System.currentTimeMillis(),
                                                                                   System.currentTimeMillis()
                                                                                           + ONE_MINUTE_IN_MS,
                                                                                   testPublicKeyId,
                                                                                   false);
        when(localCapabilitiesDirectory.lookup(eq(toParticipantId), any(DiscoveryQos.class))).thenReturn(discoveryEntry);
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
