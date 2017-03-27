package io.joynr.messaging.info;

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

import java.net.URI;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class ChannelTest {

    @Mock
    BounceProxyInformation mock;

    @Test
    public void testCreationWithoutBounceProxyInformation() {

        try {
            new Channel(null, "channel-123", URI.create("http://joyn.de/channel-123"));
            Assert.fail("Should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreationWithoutChannelId() {

        try {
            new Channel(mock, null, URI.create("http://joyn.de/channel-123"));
            Assert.fail("Should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreationWithoutChannelLocation() {

        try {
            new Channel(mock, "channel-123", null);
            Assert.fail("Should throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
        }
    }

    @Test
    public void testCreation() {

        Channel channel = new Channel(mock, "channel-123", URI.create("http://joyn.de/channel-123"));

        Assert.assertEquals("channel-123", channel.getChannelId());
        Assert.assertEquals("http://joyn.de/channel-123", channel.getLocation().toString());
        Assert.assertEquals(mock, channel.getBounceProxy());
    }
}
