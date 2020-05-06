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
package io.joynr.jeeintegration.httpbridge;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.jeeintegration.httpbridge.HttpBridgeEndpointRegistryClient.EndpointRegistryUriHolder;
import io.joynr.jeeintegration.httpbridge.HttpBridgeEndpointRegistryClient;

/**
 * Unit tests for the {@link HttpBridgeEndpointRegistryClient}.
 */
public class HttpBridgeEndpointRegistryClientTest {

    private static final Logger logger = LoggerFactory.getLogger(HttpBridgeEndpointRegistryClientTest.class);

    @Test
    public void testRegister() throws Exception {
        HttpClient httpClient = mock(HttpClient.class);
        HttpResponse response = mock(HttpResponse.class);
        StatusLine statusLine = mock(StatusLine.class);
        when(httpClient.execute(any())).thenReturn(response);
        when(response.getStatusLine()).thenReturn(statusLine);
        when(statusLine.getStatusCode()).thenReturn(201);
        String endpointRegistryUri = "http://localhost";
        EndpointRegistryUriHolder endpointRegistryUriHolder = new EndpointRegistryUriHolder(endpointRegistryUri);
        ScheduledExecutorService scheduledExecutorService = mock(ScheduledExecutorService.class);
        when(scheduledExecutorService.schedule(any(Runnable.class),
                                               anyLong(),
                                               eq(TimeUnit.SECONDS))).thenAnswer(new Answer<Object>() {
                                                   @Override
                                                   public Object answer(InvocationOnMock invocationOnMock) throws Throwable {
                                                       logger.info("About to call scheduled task.");
                                                       ((Runnable) invocationOnMock.getArguments()[0]).run();
                                                       return null;
                                                   }
                                               });

        HttpBridgeEndpointRegistryClient subject = new HttpBridgeEndpointRegistryClient(httpClient,
                                                                                        endpointRegistryUriHolder,
                                                                                        scheduledExecutorService);

        subject.register("http://endpoint:8080", "channel-id-1");

        ArgumentCaptor<HttpPost> captor = ArgumentCaptor.forClass(HttpPost.class);
        Mockito.verify(httpClient).execute(captor.capture());
        HttpPost httpPost = captor.getValue();
        assertNotNull(httpPost);
        assertEquals(endpointRegistryUri, httpPost.getURI().toString());
        assertNotNull(httpPost.getEntity());
        assertTrue(httpPost.getEntity() instanceof StringEntity);
        ObjectMapper objectMapper = new ObjectMapper();
        JsonNode json = objectMapper.readTree(((StringEntity) httpPost.getEntity()).getContent());
        assertNotNull(json);
        assertEquals("http://endpoint:8080", json.get("endpointUrl").asText());
        assertEquals("channel-id-1", json.get("topic").asText());
    }

}
