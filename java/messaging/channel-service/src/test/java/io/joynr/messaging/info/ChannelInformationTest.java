package io.joynr.messaging.info;

/*
 * #%L
 * joynr::java::messaging::channel-service
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class ChannelInformationTest {

    @Mock
    BounceProxyInformation mock;

    @Test
    public void testUriResolution() {

        Mockito.when(mock.getLocation()).thenReturn(URI.create("http://joyn.testbaseuri.io/"));
        Mockito.when(mock.getInstanceId()).thenReturn("xyz");

        ChannelInformation ci = new ChannelInformation(mock, "channel-123");

        Assert.assertEquals("http://joyn.testbaseuri.io/channels/channel-123;jsessionid=.xyz", ci.getLocation()
                                                                                                 .toString());
    }

    @Test
    public void testUriResolutionWithoutSlash() {

        Mockito.when(mock.getLocation()).thenReturn(URI.create("http://joyn.testbaseuri.io/"));
        Mockito.when(mock.getInstanceId()).thenReturn("xyz");

        ChannelInformation ci = new ChannelInformation(mock, "channel-123");

        Assert.assertEquals("http://joyn.testbaseuri.io/channels/channel-123;jsessionid=.xyz", ci.getLocation()
                                                                                                 .toString());
    }

    @Test
    public void testUriResolutionWithPath() {

        Mockito.when(mock.getLocation()).thenReturn(URI.create("http://joyn.testbaseuri.io/bounceproxy/"));
        Mockito.when(mock.getInstanceId()).thenReturn("xyz");

        ChannelInformation ci = new ChannelInformation(mock, "channel-123");

        Assert.assertEquals("http://joyn.testbaseuri.io/bounceproxy/channels/channel-123;jsessionid=.xyz",
                            ci.getLocation().toString());
    }

}
