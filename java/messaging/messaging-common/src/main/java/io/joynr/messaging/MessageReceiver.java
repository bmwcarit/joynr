package io.joynr.messaging;

/*
 * #%L
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

import java.util.concurrent.Future;

/**
 * Messaging facade.
 */
public interface MessageReceiver {

    String getChannelId();

    /**
     *
     * @param clear
     *            indicates whether the messageListener should be dropped and the channel closed
     */
    void shutdown(boolean clear);

    boolean isChannelCreated();

    boolean deleteChannel();

    boolean isStarted();

    void suspend();

    void resume();

    /**
     * @param messageArrivedListener the listener to be informed about received messages and errors 
     * @param receiverStatusListeners list of status listeners providing callbacks
     * for successful or failed start
     * @return a future that signals when the receiver is ready to be used.
     */
    Future<Void> start(MessageArrivedListener messageArrivedListener, ReceiverStatusListener... receiverStatusListeners);
}
