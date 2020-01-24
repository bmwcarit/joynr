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

import java.util.Optional;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.servlet.ServletModule;
import com.jayway.restassured.response.Response;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

/**
 * Tests the RESTful interface for the test cases in sequence diagram <b>Channel
 * Set Up</b>.
 * 
 * @author christina.strobel
 * 
 */
@RunWith(MockitoJUnitRunner.class)
public class ChannelSetUpTest extends AbstractChannelSetUpTest {

    @Mock
    ChannelService mock;

    @Override
    protected ServletModule getServletTestModule() {

        return new ServletModule() {

            @Override
            protected void configureServlets() {

                bind(ChannelService.class).toInstance(mock);

                bind(ChannelServiceRestAdapter.class);

                serve("/some-channel-service/*").with(GuiceContainer.class);
            }

        };
    }

    @Test
    public void testCreateChannelThatIsAlreadyKnown() {

        Mockito.when(mock.getChannel("channel-123"))
               .thenReturn(Optional.of(createChannel("X.Y", "http://joyn-bpX.de/bp", "channel-123")));

        Response response = //
                given(). //
                       when().post(serverUrl + "?ccid=channel-123");

        assertEquals(200 /* OK */, response.getStatusCode());
        assertEquals("http://joyn-bpX.de/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("http://joyn-bpX.de/bp/channels/channel-123", response.getBody().asString());
        assertEquals("X.Y", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verifyNoMoreInteractions(mock);
    }

    @Test
    public void testCreateNewChannel() {

        Mockito.when(mock.getChannel("channel-123")).thenReturn(Optional.empty());
        Mockito.when(mock.createChannel("channel-123", null))
               .thenReturn(createChannel("0.0", "http://joyn-bp0.de/bp", "channel-123"));

        Response response = //
                given(). //
                       when().post(serverUrl + "?ccid=channel-123");

        assertEquals(201 /* Created */, response.getStatusCode());
        assertEquals("http://joyn-bp0.de/bp/channels/channel-123", response.getHeader("Location"));
        assertEquals("http://joyn-bp0.de/bp/channels/channel-123", response.getBody().asString());
        assertEquals("0.0", response.getHeader("bp"));
        Mockito.verify(mock).getChannel("channel-123");
        Mockito.verify(mock).createChannel("channel-123", null);
        Mockito.verifyNoMoreInteractions(mock);
    }

}
