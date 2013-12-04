package io.joynr.messaging.service;

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

import io.joynr.messaging.info.ChannelInformation;

import java.util.List;

import org.atmosphere.jersey.Broadcastable;

/**
 * Interface for the delegate implementation for channel service.
 * 
 * @author christina.strobel
 * 
 */
public interface ChannelServiceDelegate {

    /**
     * Returns a list of registered channels.
     * 
     * @return
     */
    public List<ChannelInformation> listChannels();

    /**
     * Retrieves information such as URL used to post messages to this channel
     * and ID of the bounce proxy that the channel has been assigned to.
     * 
     * @param ccid
     *            the channel to communication on
     * @return information of the bounce proxy or <code>null</code> if the
     *         channel is not registered.
     */
    public ChannelInformation getChannelInformation(String ccid);

    /**
     * Opens a long poll channel.
     * 
     * @param ccid
     * @param cacheIndex
     * @param atmosphereTrackingId
     * @return
     */
    public Broadcastable openChannel(String ccid, Integer cacheIndex, String atmosphereTrackingId);

    /**
     * Creates a channel for a channelId
     * 
     * @param ccid
     * @param trackingId
     */
    public ChannelInformation createChannel(String ccid, String trackingId);

    /**
     * Deletes a channel for a channelId
     * 
     * @param ccid
     */
    public void deleteChannel(String ccid);

}
