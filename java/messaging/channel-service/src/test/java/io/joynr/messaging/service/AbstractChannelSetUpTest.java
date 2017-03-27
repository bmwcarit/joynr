package io.joynr.messaging.service;

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

import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.Channel;

import java.net.URI;

import org.junit.Before;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public abstract class AbstractChannelSetUpTest extends AbstractServiceInterfaceTest {

    protected String serverUrl;

    @Before
    public void setUp() throws Exception {
        super.setUp();

        serverUrl = String.format("%s/some-channel-service/channels", getServerUrlWithoutPath());
    }

    protected Channel createChannel(String bpId, String baseUrl, String ccid) {

        BounceProxyInformation bounceProxy = new BounceProxyInformation(bpId, URI.create(baseUrl));
        Channel channel = new Channel(bounceProxy, ccid, URI.create(baseUrl + "/channels/" + ccid));

        return channel;
    }

}
