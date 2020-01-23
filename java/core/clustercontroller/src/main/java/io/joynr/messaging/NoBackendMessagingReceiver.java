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

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Future;

import com.google.inject.Singleton;

/**
 * A Dummy implementation that does not perform any backend communication
 *
 */

@Singleton
public class NoBackendMessagingReceiver implements MessageReceiver {

    @Override
    public String getChannelId() {
        return "null";
    }

    @Override
    public void shutdown(boolean clear) {

    }

    @Override
    public boolean deleteChannel() {
        return true;
    }

    @Override
    public boolean isStarted() {
        return true;
    }

    @Override
    public void suspend() {
    }

    @Override
    public void resume() {
    }

    @Override
    public boolean isReady() {
        return true;
    }

    @Override
    public Future<Void> start(MessageArrivedListener messageArrivedListener,
                              ReceiverStatusListener... statusListeners) {
        return CompletableFuture.allOf();
    }
}
