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

/**
 * Class representing a messaging channel in a channel service.
 *
 * @author christina.strobel
 *
 */
public class Channel {

    private String channelId;
    private BounceProxyInformation bounceProxy;

    /**
     * Absolute URI for the channel.
     */
    private URI channelLocation;

    /**
     * Creates a new channel.
     *
     * @param bounceProxy
     *            the bounce proxy that handles communication for this channel.
     *            Must not be <code>null</code>.
     * @param channelId
     *            the channel identifier. Must not be <code>null</code>.
     * @param channelLocation
     *            the URL of the channel which is used for cluster controllers
     *            for messaging on this channel. Must not be <code>null</code>.
     *
     * @throws IllegalArgumentException
     *             if any parameter is <code>null</code>
     */
    public Channel(BounceProxyInformation bounceProxy, String channelId, URI channelLocation) {

        if (bounceProxy == null) {
            throw new IllegalArgumentException("Parameter 'bounceProxy' must not be null");
        }

        if (channelId == null) {
            throw new IllegalArgumentException("Parameter 'channelId' must not be null");
        }

        if (channelLocation == null) {
            throw new IllegalArgumentException("Parameter 'channelLocation' must not be null");
        }

        this.channelId = channelId;
        this.bounceProxy = bounceProxy;
        this.channelLocation = channelLocation;
    }

    /**
     * Returns the identifier of this channel.
     *
     * @return channel ID. Should not be <code>null</code>.
     */
    public String getChannelId() {
        return this.channelId;
    }

    /**
     * Returns information about the bounce proxy handling communication for
     * this channel.
     *
     * @return information about the bounce proxy. Should not be <code>null</code>.
     */
    public BounceProxyInformation getBounceProxy() {
        return this.bounceProxy;
    }

    /**
     * Returns the location of the channel as it is used by cluster controllers
     * to send messages to and retrieve messages from the channel.
     *
     * @return a location. Should not be <code>null</code>.
     */
    public URI getLocation() {
        return this.channelLocation;
    }
}
