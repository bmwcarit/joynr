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
package io.joynr.messaging.bounceproxy.controller;

import java.net.URI;
import java.util.LinkedList;
import java.util.List;
import java.util.Optional;

import org.atmosphere.jersey.Broadcastable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.strategy.ChannelAssignmentStrategy;
import io.joynr.messaging.info.Channel;
import io.joynr.messaging.info.ChannelInformation;
import io.joynr.messaging.info.ControlledBounceProxyInformation;
import io.joynr.messaging.service.ChannelService;

/**
 * Implementation of channel service for the bounce proxy controller.
 * 
 * @author christina.strobel
 * 
 */
public class ChannelServiceImpl implements ChannelService {

    private static final Logger log = LoggerFactory.getLogger(ChannelServiceImpl.class);

    @Inject
    private ChannelDirectory channelDirectory;

    @Inject
    private BounceProxyDirectory bounceProxyDirectory;

    @Inject
    private ChannelAssignmentStrategy strategy;

    @Inject
    RemoteBounceProxyFacade bpFacade;

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.messaging.service.ChannelServiceDelegate#listChannels()
     */
    @Override
    public List<ChannelInformation> listChannels() {
        List<Channel> channels = channelDirectory.getChannels();

        List<ChannelInformation> channelInformationList = new LinkedList<ChannelInformation>();
        for (Channel channel : channels) {

            ChannelInformation channelInfo = new ChannelInformation(channel.getChannelId(), 0, Optional.of(0));
            channelInformationList.add(channelInfo);
        }
        return channelInformationList;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.service.ChannelServiceDelegate#getChannelInformation
     * (java.lang.String)
     */
    @Override
    public Optional<Channel> getChannel(String ccid) {
        return Optional.ofNullable(channelDirectory.getChannel(Optional.of(ccid)));
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.service.ChannelServiceDelegate#openChannel(java.lang
     * .String, java.lang.Integer, java.lang.String)
     */
    @Override
    public Broadcastable openChannel(String ccid, Integer cacheIndex, String atmosphereTrackingId) {
        throw new JoynrCommunicationException("No channels can't be opened on bounce proxy controller, only on bounce proxies.");
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.service.ChannelServiceDelegate#createChannel(java.
     * lang.String, java.lang.String)
     */
    @Override
    public Channel createChannel(String ccid, String trackingId) {

        try {
            // The controller has to assign the channel to a bounce proxy
            // instance.
            ControlledBounceProxyInformation bpInfo = strategy.calculateBounceProxy(ccid);

            URI channelLocation = bpFacade.createChannel(bpInfo, ccid, trackingId);

            Channel channel = new Channel(bpInfo, ccid, channelLocation);

            channelDirectory.addChannel(channel);
            bounceProxyDirectory.updateChannelAssignment(ccid, bpInfo);

            return channel;

        } catch (Exception e) {
            log.error("Could not create channel on bounce proxy: channel {}, error: {}", ccid, e.getMessage());
            throw new JoynrRuntimeException("Error creating channel on bounce proxy", e);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.service.ChannelServiceDelegate#deleteChannel(java.
     * lang.String)
     */
    @Override
    public boolean deleteChannel(String ccid) {
        return false;
    }

}
