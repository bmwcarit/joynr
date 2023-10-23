/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.proxy;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import joynr.OneWayRequest;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.junit.MockitoJUnitRunner;

import java.lang.reflect.Method;
import java.util.Set;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

@RunWith(MockitoJUnitRunner.class)
public class JoynrMessagingConnectorInvocationHandlerOneWayMethodTest
        extends AbstractJoynrMessagingConnectorInvocationHandlerTest {

    @Test
    public void testExecuteOneWayMethodShouldSucceed() {
        method = getFireAndForgetMethod("methodWithoutParameters");
        parameters = new Object[]{};
        handler.executeOneWayMethod(method, parameters);

        final ArgumentCaptor<OneWayRequest> requestCaptor = ArgumentCaptor.forClass(OneWayRequest.class);

        verify(requestReplyManager).sendOneWayRequest(eq(FROM_PARTICIPANT_ID),
                                                      eq(toDiscoveryEntries),
                                                      requestCaptor.capture(),
                                                      any(MessagingQos.class));

        final OneWayRequest request = requestCaptor.getValue();
        assertNotNull(request);
        assertEquals(method.getName(), request.getMethodName());
    }

    @Test
    public void testExecuteOneWayMethodShouldFailIfDiscoveryEntrySetIsEmpty() {
        method = getFireAndForgetMethod("methodWithoutParameters");
        parameters = new Object[]{};
        toDiscoveryEntries.clear();
        try {
            handler.executeOneWayMethod(method, parameters);
        } catch (final JoynrIllegalStateException exception) {
            assertNotNull(exception);
            assertTrue(exception.getMessage().contains("at least one participant"));
            //noinspection unchecked
            verify(requestReplyManager, never()).sendOneWayRequest(anyString(),
                                                                   any(Set.class),
                                                                   any(OneWayRequest.class),
                                                                   any(MessagingQos.class));
        }
    }

    private Method getFireAndForgetMethod(final String methodName, final Class<?>... parameterTypes) {
        return getMethod(TestFireAndForgetInterface.class, methodName, parameterTypes);
    }
}