package io.joynr.messaging.bounceproxy.controller;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import static org.junit.Assert.assertEquals;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.exception.JoynrProtocolException;
import io.joynr.messaging.bounceproxy.controller.strategy.ChannelAssignmentStrategy;
import io.joynr.messaging.info.Channel;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

import java.net.URI;

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
public class ChannelServiceImplTest {

    private ChannelServiceImpl channelService;

    @Mock
    ChannelDirectory channelDirectoryMock;

    @Mock
    BounceProxyDirectory bpDirectoryMock;

    @Mock
    ChannelAssignmentStrategy channelAssignmentStrategyMock;

    @Mock
    RemoteBounceProxyFacade bpMock;

    @Before
    public void setUp() {

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(ChannelDirectory.class).toInstance(channelDirectoryMock);
                bind(ChannelAssignmentStrategy.class).toInstance(channelAssignmentStrategyMock);
                bind(BounceProxyDirectory.class).toInstance(bpDirectoryMock);
                bind(RemoteBounceProxyFacade.class).toInstance(bpMock);
            }

        });

        channelService = injector.getInstance(ChannelServiceImpl.class);
    }

    @Test
    public void testCreateChannel() throws JoynrProtocolException {

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y",
                                                                                       URI.create("http://bpX.de"));
        Mockito.when(channelAssignmentStrategyMock.calculateBounceProxy("channel-123")).thenReturn(bpInfo);
        Mockito.when(bpMock.createChannel(bpInfo, "channel-123", "trackingId-xyz"))
               .thenReturn(URI.create("http://bpX.de/channels/channel-123"));

        Channel channel = channelService.createChannel("channel-123", "trackingId-xyz");

        assertEquals("channel-123", channel.getChannelId());
        assertEquals("X.Y", channel.getBounceProxy().getId());
        assertEquals("http://bpX.de", channel.getBounceProxy().getLocation().toString());
        assertEquals("http://bpX.de/channels/channel-123", channel.getLocation().toString());
        Mockito.verify(channelDirectoryMock, Mockito.times(1)).addChannel(channel);
        Mockito.verify(bpDirectoryMock).updateChannelAssignment("channel-123", bpInfo);
    }

}
