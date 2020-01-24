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
package io.joynr.messaging;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.routing.MulticastAddressCalculator;
import io.joynr.messaging.routing.TransportReadyListener;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

public class LongPollingHttpMulticastAddressCalculator implements MulticastAddressCalculator {

    private static final Logger logger = LoggerFactory.getLogger(LongPollingHttpMulticastAddressCalculator.class);

    private static final String UTF_8 = "UTF-8";

    private ChannelAddress globalAddress;

    @Inject
    public LongPollingHttpMulticastAddressCalculator(LongPollingHttpGlobalAddressFactory longPollingHttpGlobalAddressFactory) {
        longPollingHttpGlobalAddressFactory.registerGlobalAddressReady(new TransportReadyListener() {
            @Override
            public void transportReady(Optional<Address> address) {
                if (address.isPresent() && (address.get() instanceof ChannelAddress)) {
                    globalAddress = (ChannelAddress) address.get();
                }
            }
        });
    }

    @Override
    public Set<Address> calculate(ImmutableMessage message) {
        if (true) {
            throw new UnsupportedOperationException("Multicasts are not yet supported for HTTP long polling.");
        }
        Set<Address> multicastAddresses = new HashSet<>();
        if (globalAddress != null) {
            try {
                String multicastChannelId = URLEncoder.encode(message.getSender() + "/" + message.getRecipient(),
                                                              UTF_8);
                multicastAddresses.add(new ChannelAddress(globalAddress.getMessagingEndpointUrl(), multicastChannelId));
            } catch (UnsupportedEncodingException e) {
                logger.error("Unable to encode multicast channel from message {}", message, e);
            }
        } else {
            logger.warn("Unable to calculate multicast address for message {} because no global address was found for long polling.",
                        message);
        }
        return multicastAddresses;
    }

    @Override
    public boolean supports(String transport) {
        return LongPollingHttpGlobalAddressFactory.SUPPORTED_TRANSPORT_LONGPOLLING.equals(transport);
    }

    @Override
    public boolean createsGlobalTransportAddresses() {
        return true;
    }
}
