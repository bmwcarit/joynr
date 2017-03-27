package io.joynr.messaging.bounceproxy.service;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.messaging.bounceproxy.LongPollingMessagingDelegate;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.Channel;

import java.net.URI;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

@RunWith(MockitoJUnitRunner.class)
public class DefaultBounceProxyChannelServiceImplTest {

    private DefaultBounceProxyChannelServiceImpl channelService;

    @Mock
    private LongPollingMessagingDelegate longPollingDelegateMock;

    @Mock
    private BounceProxyInformation bpInfoMock;

    @Before
    public void setUp() {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(LongPollingMessagingDelegate.class).toInstance(longPollingDelegateMock);
                bind(BounceProxyInformation.class).toInstance(bpInfoMock);
            }
        });

        channelService = injector.getInstance(DefaultBounceProxyChannelServiceImpl.class);
    }

    @Test
    public void testCreateChannel() {

        Mockito.when(bpInfoMock.getId()).thenReturn("X.Y");
        Mockito.when(bpInfoMock.getLocation()).thenReturn(URI.create("http://www.joynr.io"));
        Mockito.when(longPollingDelegateMock.createChannel("channel-123", "trackingId-123"))
               .thenReturn("channels/channel-123/");

        Channel channel = channelService.createChannel("channel-123", "trackingId-123");

        Assert.assertEquals("channel-123", channel.getChannelId());
        Assert.assertEquals("http://www.joynr.io/channels/channel-123/", channel.getLocation().toString());
        Assert.assertEquals(bpInfoMock, channel.getBounceProxy());
    }

    @Test
    public void testDeleteChannelSuccessful() {

        Mockito.when(longPollingDelegateMock.deleteChannel("channel-123")).thenReturn(true);

        Assert.assertTrue(channelService.deleteChannel("channel-123"));
    }

    @Test
    public void testDeleteChannelFails() {

        Mockito.when(longPollingDelegateMock.deleteChannel("channel-123")).thenReturn(false);

        Assert.assertFalse(channelService.deleteChannel("channel-123"));
    }
}
