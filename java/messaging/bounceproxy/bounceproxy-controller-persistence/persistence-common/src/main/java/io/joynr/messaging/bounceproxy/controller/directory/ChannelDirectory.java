package io.joynr.messaging.bounceproxy.controller.directory;

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

import io.joynr.messaging.info.Channel;

import java.util.List;

import javax.annotation.CheckForNull;

/**
 * Interface for a directory or database, respectively, that stores channel URLs for each channel ID.
 * 
 * @author christina.strobel
 *
 */
public interface ChannelDirectory {

    /**
     * Returns a list of all channels currently stored in the directory.
     * 
     * @return list of all channels currently stored in the directory.
     */
    public List<Channel> getChannels();

    /**
     * Returns the channel for a certain channel ID.
     * 
     * @param ccid the channel ID
     * @return the channel ID or <code>null</code> if no channel is stored for the ID.
     */
    @CheckForNull
    public Channel getChannel(String ccid);

    /**
     * Adds a new channel to the directory.
     * 
     * @param channel the channel to be added
     */
    public void addChannel(Channel channel);

}
