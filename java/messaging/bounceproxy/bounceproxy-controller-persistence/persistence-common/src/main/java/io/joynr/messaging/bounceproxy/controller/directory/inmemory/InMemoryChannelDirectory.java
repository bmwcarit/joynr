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
package io.joynr.messaging.bounceproxy.controller.directory.inmemory;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Optional;

import com.google.inject.Singleton;

import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.info.Channel;

/**
 * @author christina.strobel
 *
 */
@Singleton
public class InMemoryChannelDirectory implements ChannelDirectory {

    private HashMap<String, Channel> channels = new HashMap<String, Channel>();

    @Override
    public List<Channel> getChannels() {
        return new LinkedList<Channel>(channels.values());
    }

    @Override
    public Channel getChannel(Optional<String> ccid) {
        return channels.get(ccid.isPresent() ? ccid.get() : null);
    }

    @Override
    public void addChannel(Channel channel) {
        channels.put(channel.getChannelId(), channel);
    }

}
