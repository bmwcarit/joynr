package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::channel-service
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

/**
 * Interface for service implementations of {@link ChannelRecoveryServiceRestAdapter}.
 * 
 * @author christina.strobel
 * 
 */
public interface ChannelRecoveryService extends ChannelService {

    /**
     * Tests if a bounce proxy handling a channel is responding to requests.
     * 
     * @param ccid
     *            the ID of the channel
     * @return <code>true</code> if the bounce proxy for the channel is
     *         reachable, <code>false</code> if not.
     */
    public boolean isBounceProxyForChannelResponding(String ccid);

    /**
     * Tries to recover a channel on the same bounce proxy instance that it has
     * been assigned to previously.
     * 
     * This is basically the same as {@link #createChannel(String, String)}
     * without assigning the channel to the bounce proxy.
     * 
     * @param ccid
     *            the ID of the channel
     * @param atmosphereTrackingId
     *            the atmosphere tracking id
     * @return
     *            the recovered channel
     */
    public Channel recoverChannel(String ccid, String atmosphereTrackingId);

}
