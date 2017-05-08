package io.joynr.messaging;

/*
 * #%L
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

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import com.google.inject.Inject;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import io.joynr.messaging.routing.TransportReadyListener;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class LongPollingHttpMulticastAddressCalculator implements MulticastAddressCalculator {

    private static final Logger logger = LoggerFactory.getLogger(LongPollingHttpMulticastAddressCalculator.class);

    private static final String UTF_8 = "UTF-8";

    private ChannelAddress globalAddress;

    @Inject
    public LongPollingHttpMulticastAddressCalculator(LongPollingHttpGlobalAddressFactory longPollingHttpGlobalAddressFactory) {
        longPollingHttpGlobalAddressFactory.registerGlobalAddressReady(new TransportReadyListener() {
            @Override
            public void transportReady(Address address) {
                if (address instanceof ChannelAddress) {
                    globalAddress = (ChannelAddress) address;
                }
            }
        });
    }

    @Override
    public Address calculate(ImmutableMessage message) {
        if (true) {
            throw new UnsupportedOperationException("Multicasts are not yet supported for HTTP long polling.");
        }
        ChannelAddress multicastAddress = null;
        if (globalAddress != null) {
            try {
                String multicastChannelId = URLEncoder.encode(message.getSender() + "/" + message.getRecipient(), UTF_8);
                multicastAddress = new ChannelAddress(globalAddress.getMessagingEndpointUrl(), multicastChannelId);
            } catch (UnsupportedEncodingException e) {
                logger.error("Unable to encode multicast channel from message {}", message, e);
            }
        } else {
            logger.warn("Unable to calculate multicast address for message {} because no global address was found for long polling.",
                        message);
        }
        return multicastAddress;
    }

    @Override
    public boolean supports(String transport) {
        return LongPollingHttpGlobalAddressFactory.SUPPORTED_TRANSPORT_LONGPOLLING.equals(transport);
    }
}
