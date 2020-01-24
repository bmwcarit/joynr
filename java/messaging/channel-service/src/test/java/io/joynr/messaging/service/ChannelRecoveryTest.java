/*
 * #%L
 * joynr::java::messaging::channel-service
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
package io.joynr.messaging.service;

import static com.jayway.restassured.RestAssured.given;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import java.util.Optional;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import com.google.inject.servlet.ServletModule;
import com.jayway.restassured.response.Response;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 * @author christina.strobel
 * 
 */
public class ChannelRecoveryTest extends AbstractChannelSetUpTest {

    private String serverUrl;

    @Mock
    ChannelRecoveryService mock;

    @Mock
    ChannelErrorNotifier notifierMock;

    @Override
    protected ServletModule getServletTestModule() {

        return new ServletModule() {

            @Override
            protected void configureServlets() {

                bind(ChannelRecoveryService.class).toInstance(mock);
                bind(ChannelErrorNotifier.class).toInstance(notifierMock);

                bind(ChannelRecoveryServiceRestAdapter.class);

                serve("/some-channel-service/*").with(GuiceContainer.class);
            }

        };
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        super.setUp();

        serverUrl = String.format("%s/some-channel-service/channels", getServerUrlWithoutPath());
    }

    @Test
    public void testErrorHandlingBpRejectingLongPollsButBpcDoesntKnowChannelId() {

        Mockito.when(mock.getChannel("channel-123")).thenReturn(Optional.empty());
        Mockito.when(mock.createChannel("channel-123", null))
               .thenReturn(createChannel("0.0", "http://joyn-bp0.muc/bp", "channel-123"));

        Response response = //
                given(). //
                       when().put(serverUrl + "/channel-123?bp=0.0&status=rejecting_long_polls");

        assertEquals(201 /* Created */, response.getStatusCode());
        assertEquals("http://joyn-bp0.muc/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("0.0", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verify(mock).createChannel("channel-123", null);
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testErrorHandlingBpRejectingLongPollsBecauseItWasMigrated() {

        Mockito.when(mock.getChannel("channel-123"))
               .thenReturn(Optional.of(createChannel("X.Y", "http://joyn-bpX.muc/bp", "channel-123")));

        Response response = //
                given(). //
                       when().put(serverUrl + "/channel-123?bp=A.B&status=rejecting_long_polls");

        assertEquals(200 /* OK */, response.getStatusCode());
        assertEquals("http://joyn-bpX.muc/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("X.Y", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testErrorHandlingBpRejectingLongPollsBecauseItLostData() {

        Mockito.when(mock.getChannel("channel-123"))
               .thenReturn(Optional.of(createChannel("A.B", "http://joyn-bpA.muc/bp", "channel-123")));
        Mockito.when(mock.recoverChannel("channel-123", "trackingId-xyz"))
               .thenReturn(createChannel("A.B", "http://joyn-bpA.muc/bp", "channel-123"));

        Response response = //
                given(). //
                       when(). //
                       header(ChannelServiceConstants.X_ATMOSPHERE_TRACKING_ID, "trackingId-xyz"). //
                       put(serverUrl + "/channel-123?bp=A.B&status=rejecting_long_polls");

        assertEquals(204 /* No Content */, response.getStatusCode());
        assertNull(response.getHeader("Location"));
        assertNull(response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verify(mock).recoverChannel("channel-123", "trackingId-xyz");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testErrorHandlingBpUnreachableAndBpcDoesntKnowTheChannel() {

        Mockito.when(mock.getChannel("channel-123")).thenReturn(Optional.empty());
        Mockito.when(mock.createChannel("channel-123", null))
               .thenReturn(createChannel("X.Y", "http://joyn-bpX.muc/bp", "channel-123"));

        Response response = //
                given(). //
                       when().put(serverUrl + "/channel-123?bp=X.Y&status=unreachable");

        assertEquals(201 /* Created */, response.getStatusCode());
        assertEquals("http://joyn-bpX.muc/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("X.Y", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verify(mock).createChannel("channel-123", null);
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testErrorHandlingBpUnreachableBecauseItWasMigrated() {

        Mockito.when(mock.getChannel("channel-123"))
               .thenReturn(Optional.of(createChannel("X.Y", "http://joyn-bpX.muc/bp", "channel-123")));

        Response response = //
                given(). //
                       when().put(serverUrl + "/channel-123?bp=A.B&status=unreachable");

        assertEquals(200 /* OK */, response.getStatusCode());
        assertEquals("http://joyn-bpX.muc/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("X.Y", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testErrorHandlingBpUnreachableForClusterControllersOnly() {

        Mockito.when(mock.getChannel("channel-123"))
               .thenReturn(Optional.of(createChannel("X.Y", "http://joyn-bpX.muc/bp", "channel-123")));
        Mockito.when(mock.isBounceProxyForChannelResponding("channel-123")).thenReturn(true);

        Response response = //
                given(). //
                       queryParam("bp", "X.Y").and().queryParam("status", "unreachable").when().put(serverUrl
                               + "/channel-123");

        assertEquals(204 /* No Content */, response.getStatusCode());
        assertNull(response.getHeader("Location"));
        assertNull(response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verify(mock).isBounceProxyForChannelResponding("channel-123");
        Mockito.verifyNoMoreInteractions(mock);
        Mockito.verify(notifierMock).alertBounceProxyUnreachable("channel-123",
                                                                 "X.Y",
                                                                 "127.0.0.1",
                                                                 "Bounce Proxy unreachable for Cluster Controller");
    }

    @Test
    public void testErrorHandlingBpUnreachable() {

        Mockito.when(mock.getChannel("channel-123"))
               .thenReturn(Optional.of(createChannel("X.Y", "http://joyn-bpX.muc/bp", "channel-123")));
        Mockito.when(mock.isBounceProxyForChannelResponding("channel-123")).thenReturn(false);
        Mockito.when(mock.createChannel("channel-123", null))
               .thenReturn(createChannel("1.1", "http://joyn-bp1.muc/bp", "channel-123"));

        Response response = //
                given(). //
                       queryParam("bp", "X.Y").and().queryParam("status", "unreachable").when().put(serverUrl
                               + "/channel-123");

        assertEquals(201 /* Created */, response.getStatusCode());
        assertEquals("http://joyn-bp1.muc/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("1.1", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verify(mock).isBounceProxyForChannelResponding("channel-123");
        Mockito.verify(mock).createChannel("channel-123", null);
        Mockito.verifyNoMoreInteractions(mock);
        Mockito.verify(notifierMock).alertBounceProxyUnreachable("channel-123",
                                                                 "X.Y",
                                                                 "127.0.0.1",
                                                                 "Bounce Proxy unreachable for Channel Service");
    }

}
