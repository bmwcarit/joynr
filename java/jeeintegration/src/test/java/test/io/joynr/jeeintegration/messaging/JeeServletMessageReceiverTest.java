/**
 *
 */
package test.io.joynr.jeeintegration.messaging;

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

import io.joynr.common.ExpiryDate;
import io.joynr.jeeintegration.httpbridge.HttpBridgeRegistryClient;
import io.joynr.jeeintegration.messaging.JeeServletMessageReceiver;
import io.joynr.messaging.MessageArrivedListener;
import joynr.ImmutableMessage;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import java.util.concurrent.CompletionStage;
import java.util.function.Consumer;

import static java.lang.Boolean.TRUE;
import static org.junit.Assert.*;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.*;

/**
 * Unit tests for the {@link JeeServletMessageReceiver}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeServletMessageReceiverTest {

    private String channelId = "channel-1";

    private String contextRoot = "/";

    private String hostPath = "http://localhost";

    @Mock
    private MessageArrivedListener messageArrivedListener;

    @Mock
    private HttpBridgeRegistryClient httpBridgeRegistryClient;

    private JeeServletMessageReceiver subject;

    @Before
    public void setup() {
        subject = new JeeServletMessageReceiver(channelId,
                                                contextRoot,
                                                hostPath,
                                                httpBridgeRegistryClient,
                                                TRUE.toString());
    }

    @Test
    public void testGetChannelId() {
        String result = subject.getChannelId();
        assertNotNull(result);
        assertEquals(channelId, result);
    }

    @Test
    public void testIsChannelCreated() {
        assertFalse(subject.isReady());
    }

    @Test
    public void testDeleteChannel() {
        assertFalse(subject.deleteChannel());
    }

    @Test
    public void testIsStarted() {
        assertFalse(subject.isStarted());
    }

    @Test
    public void testSuspend() {
        subject.suspend();
    }

    @Test
    public void testResume() {
        subject.resume();
    }

    @Test
    public void testSwitchToLongPolling() {
        assertFalse(subject.switchToLongPolling());
    }

    @Test
    public void testStart() {
        start();
        assertTrue(subject.isStarted());
        assertTrue(subject.isReady());
        verify(httpBridgeRegistryClient).register(Mockito.any(), Mockito.any());
    }

    @Test
    public void testShutdown() {
        start();
        subject.shutdown(true);
    }

    @Test
    public void testReceive() {
        start();
        ImmutableMessage message = Mockito.mock(ImmutableMessage.class);
        when(message.isTtlAbsolute()).thenReturn(true);
        when(message.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        subject.receive(message);
        verify(messageArrivedListener).messageArrived(message);
    }

    @Test
    public void testOnError() {
        start();
        ImmutableMessage message = Mockito.mock(ImmutableMessage.class);
        Throwable error = new RuntimeException();
        subject.onError(message, error);
        verify(messageArrivedListener).error(message, error);
    }

    @SuppressWarnings("unchecked")
    private void start() {
        CompletionStage<Void> completionStage = mock(CompletionStage.class);
        when(httpBridgeRegistryClient.register(anyString(), anyString())).thenReturn(completionStage);
        when(completionStage.thenAccept(any(Consumer.class))).thenAnswer(new Answer<CompletionStage<Void>>() {
            @SuppressWarnings("rawtypes")
            @Override
            public CompletionStage<Void> answer(InvocationOnMock invocationOnMock) throws Throwable {
                ((Consumer) invocationOnMock.getArguments()[0]).accept(null);
                return mock(CompletionStage.class);
            }
        });
        subject.start(messageArrivedListener);
    }

}
