package io.joynr.dispatching;

import java.util.List;
import java.util.concurrent.Future;

import com.google.common.collect.Lists;
import com.google.common.util.concurrent.Futures;
import com.google.inject.Singleton;

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

import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.ReceiverStatusListener;
import joynr.ImmutableMessage;

/**
 * CommunicationManagerMock used in DispatcherTest.java to simulate the HttpCommunicationManager
 *
 */

@Singleton
public class MessageReceiverMock implements MessageReceiver {

    private List<ImmutableMessage> receivedMessages = Lists.newArrayList();

    private MessageArrivedListener messageArrivedListener;
    private boolean blockInitialisation = false;
    private boolean started = false;

    @Override
    public String getChannelId() {
        return "abc";
    }

    public void receiveMessage(ImmutableMessage message) {
        receivedMessages.add(message);
        if (messageArrivedListener != null) {
            messageArrivedListener.messageArrived(message);
        }
    }

    public List<ImmutableMessage> getReceivedMessages() {
        return receivedMessages;
    }

    @Override
    public void shutdown(boolean clear) {

    }

    @Override
    public boolean deleteChannel() {
        return false;
        // TODO Auto-generated method stub

    }

    @Override
    public boolean isStarted() {
        return started;
    }

    @Override
    public void suspend() {
        // TODO Auto-generated method stub

    }

    @Override
    public void resume() {
        // TODO Auto-generated method stub

    }

    @Override
    public boolean isReady() {
        // TODO Auto-generated method stub
        return true;
    }

    public boolean isBlockInitialisation() {
        return blockInitialisation;
    }

    public void setBlockOnInitialisation(boolean blockInitialisation) {
        this.blockInitialisation = blockInitialisation;
    }

    @Override
    public Future<Void> start(MessageArrivedListener messageArrivedListener, ReceiverStatusListener... statusListeners) {
        this.messageArrivedListener = messageArrivedListener;
        started = true;

        if (isBlockInitialisation()) {
            // Added to check if Dispatcher blocks on addReplyCaller
            try {
                Thread.sleep(Long.MAX_VALUE);
            } catch (InterruptedException e) {

            }
        }

        for (ReceiverStatusListener statusListener : statusListeners) {
            statusListener.receiverStarted();
        }

        return Futures.immediateFuture(null);

    }
}
