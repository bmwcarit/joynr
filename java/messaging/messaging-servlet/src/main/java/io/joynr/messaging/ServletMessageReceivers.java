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

import io.joynr.dispatcher.ServletMessageReceiver;

import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;

import javax.annotation.Nullable;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ServletMessageReceivers implements IServletMessageReceivers {
    private static final Logger log = LoggerFactory.getLogger(ServletMessageReceivers.class);

    ConcurrentHashMap<String, ServletMessageReceiver> messageReceivers = new ConcurrentHashMap<String, ServletMessageReceiver>();

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.messaging.IMessageReceivers#getReceiverForChannelId(java.lang.String)
     */
    @Nullable
    @Override
    public ServletMessageReceiver getServletMessageReceiver(String channelId) {
        return messageReceivers.get(channelId);
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.messaging.IMessageReceivers#registerMessageReceiver(io.joynr.messaging
     * .MessageReceiver)
     */
    @Override
    public void registerServletMessageReceiver(ServletMessageReceiver messageReceiver, String channelId) {
        log.debug("Message Receiver registered for: " + channelId); //messageReceiver.getChannelId());
        //messageReceivers.put(messageReceiver.getChannelId(), messageReceiver);
        messageReceivers.put(channelId, messageReceiver);
    }

    @Override
    public Collection<ServletMessageReceiver> getAllServletMessageReceivers() {
        return messageReceivers.values();
    }

    @Override
    public boolean contains(final String channelId) {
        return messageReceivers.containsKey(channelId);
    }

}
