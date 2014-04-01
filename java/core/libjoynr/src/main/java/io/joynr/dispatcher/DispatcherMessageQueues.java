package io.joynr.dispatcher;

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

import io.joynr.common.ExpiryDate;

import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.util.concurrent.ThreadFactoryBuilder;

/**
 * Queue to store incoming one-way and request messages if no listener/responder for the given interface address is
 * registered.
 */

public class DispatcherMessageQueues {

    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>> listenerMessageQueue = new ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>>();
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>> responderMessageQueue = new ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>>();
    private final ScheduledExecutorService scheduler;
    private static final long CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS = 5000L;
    private static final Logger logger = LoggerFactory.getLogger(DispatcherMessageQueues.class);

    public DispatcherMessageQueues() {
        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("DispatcherMessageQueues-%d")
                                                                     .build();
        scheduler = Executors.newScheduledThreadPool(2, namedThreadFactory);
        scheduler.scheduleWithFixedDelay(new Runnable() {
            public void run() {
                updateListenerMessageQueueTTL();
            }
        }, CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS, CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS, TimeUnit.MILLISECONDS);
        scheduler.scheduleWithFixedDelay(new Runnable() {
            public void run() {
                updateResponderMessageQueueTTL();
            }
        }, CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS, CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS, TimeUnit.MILLISECONDS);

    }

    public void putOneWayMessage(String participantId, JoynrMessage message, ExpiryDate incomingTtlExpirationDate_ms) {
        putMessage(listenerMessageQueue, participantId, message, incomingTtlExpirationDate_ms);
    }

    public void putRequestMessage(String participantId, JoynrMessage message, ExpiryDate incomingTtlExpirationDate_ms) {
        putMessage(responderMessageQueue, participantId, message, incomingTtlExpirationDate_ms);
    }

    private void putMessage(ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>> queue,
                            String participantId,
                            JoynrMessage message,
                            ExpiryDate incomingTtlExpirationDate_ms) {

        if (!queue.containsKey(participantId)) {
            ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> newMessageList = new ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>();
            queue.putIfAbsent(participantId, newMessageList);
        }
        queue.get(participantId).add(new ContentWithExpiryDate<JoynrMessage>(message, incomingTtlExpirationDate_ms));
    }

    public ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> getAndRemoveOneWayMessages(String participantId) {
        removeExpiredMessagesFromQueue(listenerMessageQueue.get(participantId));
        return listenerMessageQueue.remove(participantId);
    }

    public ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> getAndRemoveRequestMessages(String participantId) {
        removeExpiredMessagesFromQueue(responderMessageQueue.get(participantId));
        return responderMessageQueue.remove(participantId);
    }

    private void updateListenerMessageQueueTTL() {
        updateQueueList(listenerMessageQueue);
    }

    private void updateResponderMessageQueueTTL() {
        updateQueueList(responderMessageQueue);
    }

    private void updateQueueList(ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>> messageQueue) {

        for (ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> value : messageQueue.values()) {
            removeExpiredMessagesFromQueue(value);
        }

    }

    private void removeExpiredMessagesFromQueue(ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> messageList) {
        if (messageList != null) {
            Iterator<ContentWithExpiryDate<JoynrMessage>> iterator = messageList.iterator();
            while (iterator.hasNext()) {
                ContentWithExpiryDate<JoynrMessage> messageItem = iterator.next();
                if (messageItem.isExpired()) {
                    logger.warn("\r\n!!!!!!!! TTL DISCARD !!!!!!\r\nremoving message from queue:\r\n"
                            + messageItem.getContent() + " because it has expired. ");
                    iterator.remove();
                }
            }
        }
    }

    public void shutdown() {
        // TODO wait for execution before shutdown is forced
        scheduler.shutdownNow();
    }

}
