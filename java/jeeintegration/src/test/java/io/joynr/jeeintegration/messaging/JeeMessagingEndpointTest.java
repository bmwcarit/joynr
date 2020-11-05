/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.jeeintegration.messaging;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriBuilder;
import javax.ws.rs.core.UriInfo;

import org.junit.Test;
import org.mockito.Mockito;

import com.google.inject.Injector;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.messaging.JeeMessagingEndpoint;
import io.joynr.util.ObjectMapper;
import joynr.MutableMessage;

/**
 * Unit tests for the {@link JeeMessagingEndpoint}.
 */
public class JeeMessagingEndpointTest {

    @Test
    public void testStatus() {
        JeeMessagingEndpoint subject = createSubject().subject;
        Response result = subject.status();
        assertNotNull(result);
        assertEquals(200, result.getStatus());
    }

    @Test
    public void testPostMessageWithoutContentType() throws Exception {
        callPostMethod((JeeMessagingEndpoint subject, String channelId, byte[] message, UriInfo uriInfo) -> {
            subject.postMessageWithoutContentType(channelId, message, uriInfo);
        });
    }

    @Test
    public void testPostMessage() throws Exception {
        callPostMethod((JeeMessagingEndpoint subject, String channelId, byte[] message, UriInfo uriInfo) -> {
            subject.postMessage(channelId, message, uriInfo);
        });
    }

    private interface PostMethodCaller {
        void call(JeeMessagingEndpoint subject, String channelId, byte[] message, UriInfo uriInfo) throws Exception;
    }

    private void callPostMethod(PostMethodCaller postMethodCaller) throws Exception {
        SubjectData subjectData = createSubject();
        JeeMessagingEndpoint subject = subjectData.subject;
        byte[] payload = new byte[]{ 1, 2, 3 };

        UriInfo uriInfo = mock(UriInfo.class);
        UriBuilder uriBuilder = mock(UriBuilder.class);
        UriBuilder pathBuilder = mock(UriBuilder.class);

        MutableMessage mutableMessage = new MutableMessage();
        mutableMessage.setSender("testSender");
        mutableMessage.setRecipient("testRecipient");
        mutableMessage.setTtlAbsolute(true);
        mutableMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000L).getValue());
        mutableMessage.setPayload(payload);

        when(uriBuilder.path("messages/" + mutableMessage.getId())).thenReturn(pathBuilder);
        when(uriInfo.getBaseUriBuilder()).thenReturn(uriBuilder);
        postMethodCaller.call(subject,
                              "channel-1",
                              mutableMessage.getImmutableMessage().getSerializedMessage(),
                              uriInfo);
        Mockito.verify(subjectData.messageReceiver).receive(Mockito.any());
    }

    private SubjectData createSubject() {
        SubjectData result = new SubjectData();
        JoynrIntegrationBean joynrIntegrationBean = mock(JoynrIntegrationBean.class);
        result.injector = mock(Injector.class);
        when(joynrIntegrationBean.getJoynrInjector()).thenReturn(result.injector);
        result.objectMapper = mock(ObjectMapper.class);
        when(result.injector.getInstance(ObjectMapper.class)).thenReturn(result.objectMapper);
        result.messageReceiver = mock(ServletMessageReceiver.class);
        when(result.injector.getInstance(ServletMessageReceiver.class)).thenReturn(result.messageReceiver);
        result.subject = new JeeMessagingEndpoint(joynrIntegrationBean);
        return result;
    }

    private static class SubjectData {
        Injector injector;
        ObjectMapper objectMapper;
        ServletMessageReceiver messageReceiver;
        JeeMessagingEndpoint subject;
    }
}
