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

/**
 * Additional information about the channel.
 * 
 * @author christina.strobel
 *
 */
public class ChannelInformation {

    private String channelId;
    private BounceProxyInformation bounceProxy;

    public ChannelInformation(BounceProxyInformation bounceProxy, String channelId) {
        this.channelId = channelId;
        this.bounceProxy = bounceProxy;
    }

    public String getChannelId() {
        return this.channelId;
    }

    public BounceProxyInformation getBounceProxy() {
        return this.bounceProxy;
    }

    public URI getLocation() {

        URI bpLocation = bounceProxy.getLocation();
        return bpLocation.resolve("channels/" + channelId + ";jsessionid=." + bounceProxy.getInstanceId());
    }
}
