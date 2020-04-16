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
package io.joynr.dispatching;

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRequestInterruptedException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.provider.ProviderCallback;
import io.joynr.provider.ProviderContainer;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.MutableMessage;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.types.DiscoveryEntryWithMetaInfo;

@Singleton
public class RequestReplyManagerImpl
        implements RequestReplyManager, DirectoryListener<ProviderContainer>, ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(RequestReplyManagerImpl.class);
    private final StatelessAsyncRequestReplyIdManager statelessAsyncRequestReplyIdManager;
    private boolean running = true;

    private List<Thread> outstandingRequestThreads = Collections.synchronizedList(new ArrayList<Thread>());
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>> requestQueue = new ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>>();
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<OneWayCallable>> oneWayRequestQueue = new ConcurrentHashMap<>();
    private ConcurrentHashMap<Request, ProviderCallback<Reply>> replyCallbacks = new ConcurrentHashMap<Request, ProviderCallback<Reply>>();

    private ReplyCallerDirectory replyCallerDirectory;
    private ProviderDirectory providerDirectory;
    private RequestInterpreter requestInterpreter;
    private MessageSender messageSender;
    private MutableMessageFactory messageFactory;

    private ConcurrentMap<String, List<ScheduledFuture<?>>> cleanupSchedulerFuturesMap;

    private ScheduledExecutorService cleanupScheduler;

    @Inject
    // CHECKSTYLE:OFF
    public RequestReplyManagerImpl(MutableMessageFactory messageFactory,
                                   ReplyCallerDirectory replyCallerDirectory,
                                   ProviderDirectory providerDirectory,
                                   MessageSender messageSender,
                                   RequestInterpreter requestInterpreter,
                                   @Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                   ShutdownNotifier shutdownNotifier,
                                   StatelessAsyncRequestReplyIdManager statelessAsyncRequestReplyIdManager) {
        // CHECKSTYLE:ON
        this.messageFactory = messageFactory;
        this.replyCallerDirectory = replyCallerDirectory;
        this.providerDirectory = providerDirectory;
        this.messageSender = messageSender;
        this.requestInterpreter = requestInterpreter;
        this.cleanupScheduler = cleanupScheduler;
        this.cleanupSchedulerFuturesMap = new ConcurrentHashMap<>();
        providerDirectory.addListener(this);
        shutdownNotifier.registerForShutdown(this);
        this.statelessAsyncRequestReplyIdManager = statelessAsyncRequestReplyIdManager;
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.dispatcher.MessageSender#sendRequest(java. lang.String, java.lang.String,
     * java.lang.Object, io.joynr.dispatcher.ReplyCaller, long, long)
     */

    @Override
    public void sendRequest(final String fromParticipantId,
                            final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                            Request request,
                            MessagingQos messagingQos) {
        MutableMessage message = messageFactory.createRequest(fromParticipantId,
                                                              toDiscoveryEntry.getParticipantId(),
                                                              request,
                                                              messagingQos);
        message.setLocalMessage(toDiscoveryEntry.getIsLocal());
        message.setStatelessAsync(request.getStatelessAsyncCallbackMethodId() != null);

        logger.debug("REQUEST call proxy: method: {}, requestReplyId: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, provider domain: {}, params: {}",
                     request.getMethodName(),
                     request.getRequestReplyId(),
                     message.getId(),
                     fromParticipantId,
                     toDiscoveryEntry.getParticipantId(),
                     toDiscoveryEntry.getDomain(),
                     request.getParams());
        messageSender.sendMessage(message);
    }

    @Override
    public Object sendSyncRequest(String fromParticipantId,
                                  DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                                  Request request,
                                  SynchronizedReplyCaller synchronizedReplyCaller,
                                  MessagingQos messagingQos) {

        if (!running) {
            throw new IllegalStateException("Request: " + request.getRequestReplyId() + " failed. SenderImpl ID: "
                    + System.identityHashCode(this) + ": joynr is shutting down");
        }

        final ArrayList<Object> responsePayloadContainer = new ArrayList<Object>(1);
        // the synchronizedReplyCaller will call notify on the responsePayloadContainer when a message arrives
        synchronizedReplyCaller.setResponseContainer(responsePayloadContainer);

        sendRequest(fromParticipantId, toDiscoveryEntry, request, messagingQos);

        long entryTime = System.currentTimeMillis();

        // saving all calling threads so that they can be interrupted at shutdown
        outstandingRequestThreads.add(Thread.currentThread());
        synchronized (responsePayloadContainer) {
            while (running && responsePayloadContainer.isEmpty()
                    && entryTime + messagingQos.getRoundTripTtl_ms() > System.currentTimeMillis()) {
                try {
                    responsePayloadContainer.wait();
                } catch (InterruptedException e) {
                    if (running) {
                        throw new JoynrRequestInterruptedException("Request: " + request.getRequestReplyId()
                                + " interrupted.");
                    }
                    throw new JoynrShutdownException("Request: " + request.getRequestReplyId()
                            + " interrupted by shutdown");
                }
            }
        }
        outstandingRequestThreads.remove(Thread.currentThread());

        if (responsePayloadContainer.isEmpty()) {
            throw new JoynrIllegalStateException("Request: " + request.getRequestReplyId()
                    + " failed unexpectedly without response.");
        }

        Object response = responsePayloadContainer.get(0);
        if (response instanceof Throwable) {
            Throwable error = (Throwable) response;
            throw new JoynrCommunicationException("Request: " + request.getRequestReplyId() + " failed: "
                    + error.getMessage(), error);
        }

        return response;
    }

    @Override
    public void sendOneWayRequest(String fromParticipantId,
                                  Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                  OneWayRequest oneWayRequest,
                                  MessagingQos messagingQos) {
        for (DiscoveryEntryWithMetaInfo toDiscoveryEntry : toDiscoveryEntries) {
            MutableMessage message = messageFactory.createOneWayRequest(fromParticipantId,
                                                                        toDiscoveryEntry.getParticipantId(),
                                                                        oneWayRequest,
                                                                        messagingQos);

            logger.debug("Send OneWayRequest: method: {}, messageId: {}, proxy participantId: {}, provider participantId: {}",
                         oneWayRequest.getMethodName(),
                         message.getId(),
                         fromParticipantId,
                         toDiscoveryEntry.getParticipantId());
            messageSender.sendMessage(message);
        }
    }

    @Override
    public void entryAdded(String participantId, ProviderContainer providerContainer) {
        ConcurrentLinkedQueue<ContentWithExpiryDate<Request>> requestList = requestQueue.remove(participantId);
        if (requestList != null) {
            for (ContentWithExpiryDate<Request> requestItem : requestList) {
                if (!requestItem.isExpired()) {
                    Request request = requestItem.getContent();
                    handleRequest(replyCallbacks.remove(request), providerContainer.getRequestCaller(), request);
                }
            }
        }
        ConcurrentLinkedQueue<OneWayCallable> oneWayCallables = oneWayRequestQueue.remove(participantId);
        if (oneWayCallables != null) {
            for (OneWayCallable oneWayCallable : oneWayCallables) {
                oneWayCallable.call();
            }
        }
        List<ScheduledFuture<?>> futuresList = cleanupSchedulerFuturesMap.remove(participantId);
        if (futuresList != null) {
            for (ScheduledFuture<?> future : futuresList) {
                future.cancel(false);
            }
        }
    }

    @Override
    public void entryRemoved(String participantId) {
        //TODO cleanup requestQueue?
    }

    @Override
    public void handleOneWayRequest(final String providerParticipantId, final OneWayRequest request, long expiryDate) {
        Callable<Void> requestHandler = new Callable<Void>() {
            @Override
            public Void call() {
                requestInterpreter.invokeMethod(providerDirectory.get(providerParticipantId).getRequestCaller(),
                                                request);
                return null;
            }
        };
        OneWayCallable oneWayCallable = new OneWayCallable(requestHandler,
                                                           ExpiryDate.fromAbsolute(expiryDate),
                                                           String.valueOf(request));
        if (providerDirectory.contains(providerParticipantId)) {
            oneWayCallable.call();
        } else {
            if (!oneWayRequestQueue.containsKey(providerParticipantId)) {
                oneWayRequestQueue.putIfAbsent(providerParticipantId, new ConcurrentLinkedQueue<OneWayCallable>());
            }
            oneWayRequestQueue.get(providerParticipantId).add(oneWayCallable);
        }
    }

    @Override
    public void handleRequest(ProviderCallback<Reply> replyCallback,
                              String providerParticipant,
                              Request request,
                              long expiryDate) {
        if (providerDirectory.contains(providerParticipant)) {
            handleRequest(replyCallback, providerDirectory.get(providerParticipant).getRequestCaller(), request);
        } else {
            queueRequest(replyCallback, providerParticipant, request, ExpiryDate.fromAbsolute(expiryDate));
            logger.info("No requestCaller found for participantId: {} queuing request message.", providerParticipant);
        }
    }

    private void handleRequest(ProviderCallback<Reply> replyCallback, RequestCaller requestCaller, Request request) {
        logger.trace("Executing request {}", request.getRequestReplyId());
        requestInterpreter.execute(replyCallback, requestCaller, request);
    }

    @Override
    public void handleReply(final Reply reply) {
        String callbackId = statelessAsyncRequestReplyIdManager.getCallbackId(reply);
        boolean stateless = !reply.getRequestReplyId().equals(callbackId);
        final ReplyCaller callBack = stateless ? replyCallerDirectory.get(callbackId)
                : replyCallerDirectory.remove(callbackId);
        if (callBack == null) {
            logger.warn("No reply caller found for id: {}", callbackId);
            return;
        }
        callBack.messageCallBack(reply);
    }

    @Override
    public void handleError(Request request, Throwable error) {
        boolean stateless = request.getStatelessAsyncCallbackMethodId() != null;
        String callbackId = stateless ? request.getStatelessAsyncCallbackMethodId() : request.getRequestReplyId();
        if (callbackId != null) {
            ReplyCaller replyCaller = stateless ? replyCallerDirectory.get(callbackId)
                    : replyCallerDirectory.remove(callbackId);
            if (replyCaller != null) {
                replyCaller.error(error);
            }
        }
    }

    private void queueRequest(final ProviderCallback<Reply> replyCallback,
                              final String providerParticipantId,
                              Request request,
                              ExpiryDate expiryDate) {

        if (!requestQueue.containsKey(providerParticipantId)) {
            ConcurrentLinkedQueue<ContentWithExpiryDate<Request>> newRequestList = new ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>();
            requestQueue.putIfAbsent(providerParticipantId, newRequestList);
        }
        final ContentWithExpiryDate<Request> requestItem = new ContentWithExpiryDate<Request>(request, expiryDate);
        requestQueue.get(providerParticipantId).add(requestItem);
        replyCallbacks.put(request, replyCallback);

        ScheduledFuture<?> cleanupSchedulerFuture = cleanupScheduler.schedule(new Runnable() {

            @Override
            public void run() {
                requestQueue.get(providerParticipantId).remove(requestItem);
                replyCallbacks.remove(requestItem.getContent());
                Request request = requestItem.getContent();
                logger.warn("TTL DISCARD. providerParticipantId: {} request method: {} because it has expired.",
                            providerParticipantId,
                            request.getMethodName());

            }
        }, expiryDate.getRelativeTtl(), TimeUnit.MILLISECONDS);
        if (!cleanupSchedulerFuturesMap.containsKey(providerParticipantId)) {
            List<ScheduledFuture<?>> cleanupSchedulerFuturesList = new LinkedList<ScheduledFuture<?>>();
            cleanupSchedulerFuturesMap.put(providerParticipantId, cleanupSchedulerFuturesList);
        }
        cleanupSchedulerFuturesMap.get(providerParticipantId).add(cleanupSchedulerFuture);
    }

    @Override
    public void shutdown() {
        for (List<ScheduledFuture<?>> futuresList : cleanupSchedulerFuturesMap.values()) {
            if (futuresList != null) {
                for (ScheduledFuture<?> future : futuresList) {
                    future.cancel(false);
                }
            }
        }
        running = false;
        synchronized (outstandingRequestThreads) {
            for (Thread thread : outstandingRequestThreads) {
                logger.debug("Shutting down. Interrupting thread: {}", thread.getName());
                thread.interrupt();
            }
        }
        providerDirectory.removeListener(this);
    }
}
