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

import java.util.List;
import java.util.Optional;

import org.atmosphere.jersey.Broadcastable;

import io.joynr.messaging.info.Channel;
import io.joynr.messaging.info.ChannelInformation;

/**
 * Interface for channel service implementation.
 * 
 * @author christina.strobel
 * 
 */
public interface ChannelService {

    /**
     * Returns a list of registered channels.
     * 
     * @return list of channel informations for registered channels
     */
    public List<ChannelInformation> listChannels();

    /**
     * Retrieves information such as URL used to post messages to this channel
     * and ID of the bounce proxy that the channel has been assigned to.
     * 
     * @param ccid
     *            the channel to communication on
     * @return Optional containing information of the bounce proxy or empty Optional if channel
     *         for the ID does not exist
     */
    public Optional<Channel> getChannel(String ccid);

    /**
     * Opens a long poll channel.
     * 
     * @param ccid channel id
     * @param cacheIndex cache index
     * @param atmosphereTrackingId tracking id for atmosphere
     * @return Broadcastable object
     */
    public Broadcastable openChannel(String ccid, Integer cacheIndex, String atmosphereTrackingId);

    /**
     * Creates a channel for a channelId
     * 
     * @param ccid channel id
     * @param trackingId tracking id
     * @return created channel
     */
    public Channel createChannel(String ccid, String trackingId);

    /**
     * Deletes a channel for a channelId
     * 
     * @param ccid channel id
     * @return <code>true</code> if the channel was actually deleted,
     *         <code>false</code> if not (e.g. because it never existed)
     */
    public boolean deleteChannel(String ccid);

}
