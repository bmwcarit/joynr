package io.joynr.messaging.bounceproxy.controller;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.info.ControlledBounceProxyInformation;
import io.joynr.messaging.bounceproxy.controller.strategy.ChannelAssignmentStrategy;
import io.joynr.messaging.bounceproxy.controller.util.ChannelUrlUtil;
import io.joynr.messaging.info.ChannelInformation;
import io.joynr.messaging.service.ChannelService;
import io.joynr.messaging.system.TimestampProvider;

import java.net.URI;
import java.util.List;

import org.atmosphere.jersey.Broadcastable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

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

    @Inject
    private TimestampProvider timestampProvider;

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.messaging.service.ChannelServiceDelegate#listChannels()
     */
    @Override
    public List<ChannelInformation> listChannels() {
        return channelDirectory.getChannels();
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.service.ChannelServiceDelegate#getChannelInformation
     * (java.lang.String)
     */
    @Override
    public ChannelInformation getChannelInformation(String ccid) {
        return channelDirectory.getChannel(ccid);
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
    public ChannelInformation createChannel(String ccid, String trackingId) {

        try {
            // The controller has to assign the channel to a bounce proxy
            // instance.
            ControlledBounceProxyInformation bpInfo = strategy.calculateBounceProxy(ccid);

            URI channelLocation = bpFacade.createChannel(bpInfo, ccid, trackingId);

            URI channelLocationForCc = ChannelUrlUtil.createChannelLocation(bpInfo, ccid, channelLocation);
            ChannelInformation channelInfo = new ChannelInformation(bpInfo, ccid, channelLocationForCc);

            channelDirectory.addChannel(channelInfo);
            bounceProxyDirectory.updateChannelAssignment(ccid, bpInfo, timestampProvider.getCurrentTime());

            return channelInfo;

        } catch (Throwable e) {
            // TODO do a more fine grained error handling catching different
            // types of errors later on. Note that from current specification,
            // the bounce proxy is not expected to reject this call (Channels
            // are assigned based on performance measures reported by the bounce
            // proxy).
            log.error("Could not create channel on bounce proxy: channel {}, error: {}", ccid, e.getMessage());

            // TODO think of maybe trying to open a channel on a different
            // bounce proxy if it didn't work for this one
            throw new JoynrException("Error creating channel on bounce proxy", e);
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
        // TODO Auto-generated method stub
        return false;
    }

}
