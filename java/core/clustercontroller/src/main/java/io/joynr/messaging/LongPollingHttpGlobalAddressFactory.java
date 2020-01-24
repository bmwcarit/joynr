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

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.http.HttpGlobalAddressFactory;
import io.joynr.messaging.http.operation.ChannelCreatedListener;
import io.joynr.messaging.http.operation.LongPollingMessageReceiver;
import io.joynr.messaging.routing.TransportReadyListener;

@Singleton
public class LongPollingHttpGlobalAddressFactory extends HttpGlobalAddressFactory implements ChannelCreatedListener {

    protected static final String SUPPORTED_TRANSPORT_LONGPOLLING = "longpolling";
    private String myChannelId;
    private String messagingEndpointUrl;
    private List<TransportReadyListener> addressReadyListeners = new ArrayList<TransportReadyListener>();

    @Inject
    public LongPollingHttpGlobalAddressFactory(@Named(MessagingPropertyKeys.CHANNELID) String myChannelId,
                                               LongPollingMessageReceiver longPollingMessageReceiver) {
        longPollingMessageReceiver.registerChannelCreatedListener(this);
        this.myChannelId = myChannelId;
    }

    @Override
    protected String getMyChannelId() {
        return myChannelId;
    }

    @Override
    protected synchronized String getMessagingEndpointUrl() {
        if (messagingEndpointUrl == null) {
            throw new IllegalStateException("bounceproxy channel for long polling not yet created");
        }
        return messagingEndpointUrl;
    }

    @Override
    public boolean supportsTransport(Optional<String> transport) {
        return SUPPORTED_TRANSPORT_LONGPOLLING.equalsIgnoreCase(transport.isPresent() ? transport.get() : null);
    }

    @Override
    public synchronized void registerGlobalAddressReady(final TransportReadyListener listener) {
        addressReadyListeners.add(listener);
        if (messagingEndpointUrl != null) {
            listener.transportReady(Optional.ofNullable(create()));
        }
    }

    protected synchronized void setMessagingEndpointUrl(String messagingEndpointUrl) {
        this.messagingEndpointUrl = messagingEndpointUrl;
    }

    @Override
    public synchronized void channelCreated(String messagingEndpointUrl) {
        setMessagingEndpointUrl(messagingEndpointUrl);
        for (TransportReadyListener listener : addressReadyListeners) {
            listener.transportReady(Optional.ofNullable(create()));
        }
    }
}
