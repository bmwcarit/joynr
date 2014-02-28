package io.joynr.messaging.bounceproxy;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.info.Channel;
import io.joynr.messaging.info.ChannelInformation;
import io.joynr.messaging.service.ChannelService;

import java.net.URI;
import java.util.List;

import javax.ws.rs.core.UriBuilder;

import org.atmosphere.jersey.Broadcastable;

import com.google.inject.Inject;

/**
 * Implementation of channel service for controlled bounce proxies.
 * 
 * @author christina.strobel
 * 
 */
public class ChannelServiceImpl implements ChannelService {

    private LongPollingMessagingDelegate longPollingDelegate;

    private final BounceProxyInformation bpInfo;

    @Inject
    public ChannelServiceImpl(LongPollingMessagingDelegate longPollingDelegate, BounceProxyInformation bpInfo) {
        this.longPollingDelegate = longPollingDelegate;
        this.bpInfo = bpInfo;
    }

    @Override
    public List<ChannelInformation> listChannels() {
        return longPollingDelegate.listChannels();
    }

    @Override
    public Channel getChannel(String ccid) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Broadcastable openChannel(String ccid, Integer cacheIndex, String atmosphereTrackingId) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Channel createChannel(String ccid, String trackingId) {

        String channelPath = longPollingDelegate.createChannel(ccid, trackingId);
        URI channelLocation = UriBuilder.fromUri(bpInfo.getLocation()).path(channelPath).build();

        return new Channel(bpInfo, ccid, channelLocation);
    }

    @Override
    public boolean deleteChannel(String ccid) {
        return longPollingDelegate.deleteChannel(ccid);
    }

}
