/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
import java.util.concurrent.CancellationException;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;
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
    private boolean shuttingDown = false;

    private List<CompletableFuture<Reply>> outstandingRequestFutures = Collections.synchronizedList(new ArrayList<CompletableFuture<Reply>>());
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>> requestQueue = new ConcurrentHashMap<>();
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<OneWayRequest>>> oneWayRequestQueue = new ConcurrentHashMap<>();
    private ConcurrentHashMap<Request, ProviderCallback<Reply>> replyCallbacks = new ConcurrentHashMap<Request, ProviderCallback<Reply>>();

    private ReplyCallerDirectory replyCallerDirectory;
    private ProviderDirectory providerDirectory;
    private RequestInterpreter requestInterpreter;
    private MessageSender messageSender;
    private MutableMessageFactory messageFactory;
    // requestQueueLock protects requestQueue/oneWayRequestQueue, providerDirectory and replyCallbacks
    private Object requestQueueLock;

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
        this.requestQueueLock = new Object();
        this.replyCallerDirectory = replyCallerDirectory;
        this.providerDirectory = providerDirectory;
        this.messageSender = messageSender;
        this.requestInterpreter = requestInterpreter;
        this.cleanupScheduler = cleanupScheduler;
        this.cleanupSchedulerFuturesMap = new ConcurrentHashMap<>();
        providerDirectory.addListener(this);
        providerDirectory.forEach(this::entryAdded);
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

        if (logger.isTraceEnabled()) {
            logger.trace("REQUEST call proxy: method: {}, requestReplyId: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, domain: {}, interfaceName: {}, {}, params: {}",
                         request.getMethodName(),
                         request.getRequestReplyId(),
                         message.getId(),
                         fromParticipantId,
                         toDiscoveryEntry.getParticipantId(),
                         toDiscoveryEntry.getDomain(),
                         toDiscoveryEntry.getInterfaceName(),
                         toDiscoveryEntry.getProviderVersion(),
                         request.getParams());
        } else {
            logger.debug("REQUEST call proxy: method: {}, requestReplyId: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, domain: {}, interfaceName: {}, {}",
                         request.getMethodName(),
                         request.getRequestReplyId(),
                         message.getId(),
                         fromParticipantId,
                         toDiscoveryEntry.getParticipantId(),
                         toDiscoveryEntry.getDomain(),
                         toDiscoveryEntry.getInterfaceName(),
                         toDiscoveryEntry.getProviderVersion());
        }
        messageSender.sendMessage(message);
    }

    @Override
    public Reply sendSyncRequest(String fromParticipantId,
                                 DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                                 Request request,
                                 SynchronizedReplyCaller synchronizedReplyCaller,
                                 MessagingQos messagingQos) {

        if (shuttingDown) {
            throw new IllegalStateException("Request: " + request.getRequestReplyId() + " failed. SenderImpl ID: "
                    + System.identityHashCode(this) + ": joynr is shutting down");
        }

        CompletableFuture<Reply> responseFuture = new CompletableFuture<>();
        // the synchronizedReplyCaller will complete the future when a message arrives
        synchronizedReplyCaller.setResponseFuture(responseFuture);

        sendRequest(fromParticipantId, toDiscoveryEntry, request, messagingQos);

        // saving all pending futures so that they can be cancelled at shutdown
        Reply response = null;
        outstandingRequestFutures.add(responseFuture);
        if (!shuttingDown) {
            try {
                response = responseFuture.get();
            } catch (InterruptedException e) {
                e.printStackTrace();
                throw new JoynrRequestInterruptedException("Request: " + request.getRequestReplyId()
                        + " interrupted unexpectedly.");
            } catch (ExecutionException e) {
                throw new JoynrCommunicationException("Request: " + request.getRequestReplyId() + " failed: "
                        + e.getMessage(), e);
            } catch (CancellationException e) {
                throw new JoynrShutdownException("Request: " + request.getRequestReplyId()
                        + " interrupted by shutdown");
            }
        }
        outstandingRequestFutures.remove(responseFuture);

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

            if (logger.isTraceEnabled()) {
                logger.trace("ONEWAYREQUEST call proxy: method: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, domain {}, interfaceName {}, {}, params: {}",
                             oneWayRequest.getMethodName(),
                             message.getId(),
                             fromParticipantId,
                             toDiscoveryEntry.getParticipantId(),
                             toDiscoveryEntry.getDomain(),
                             toDiscoveryEntry.getInterfaceName(),
                             toDiscoveryEntry.getProviderVersion(),
                             oneWayRequest.getParams());
            } else {
                logger.debug("ONEWAYREQUEST call proxy: method: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, domain {}, interfaceName {}, {}",
                             oneWayRequest.getMethodName(),
                             message.getId(),
                             fromParticipantId,
                             toDiscoveryEntry.getParticipantId(),
                             toDiscoveryEntry.getDomain(),
                             toDiscoveryEntry.getInterfaceName(),
                             toDiscoveryEntry.getProviderVersion());
            }
            messageSender.sendMessage(message);
        }
    }

    @Override
    public void entryAdded(String participantId, ProviderContainer providerContainer) {
        RequestCaller requestCaller = providerContainer.getRequestCaller();
        List<ScheduledFuture<?>> futuresList;
        ConcurrentLinkedQueue<ContentWithExpiryDate<Request>> requestList;
        LinkedList<ProviderCallback<Reply>> requestListCallbacks = new LinkedList<ProviderCallback<Reply>>();
        ConcurrentLinkedQueue<ContentWithExpiryDate<OneWayRequest>> oneWayRequestList;
        synchronized (requestQueueLock) {
            requestList = requestQueue.remove(participantId);
            if (requestList != null) {
                for (ContentWithExpiryDate<Request> requestItem : requestList) {
                    Request request = requestItem.getContent();
                    requestListCallbacks.addLast(replyCallbacks.remove(request));
                }
            }

            oneWayRequestList = oneWayRequestQueue.remove(participantId);
            futuresList = cleanupSchedulerFuturesMap.remove(participantId);
        }

        if (requestList != null) {
            for (ContentWithExpiryDate<Request> requestItem : requestList) {
                ProviderCallback<Reply> callback = requestListCallbacks.removeFirst();
                if (!requestItem.isExpired()) {
                    Request request = requestItem.getContent();
                    handleRequest(callback, requestCaller, request);
                } else {
                    logger.warn("Request {} is expired. Not executing.", requestItem.getContent().getRequestReplyId());
                }
            }
        }

        if (oneWayRequestList != null) {
            for (ContentWithExpiryDate<OneWayRequest> requestItem : oneWayRequestList) {
                OneWayRequest request = requestItem.getContent();
                if (!requestItem.isExpired()) {
                    handleOneWayRequest(requestCaller, request);
                } else {
                    logger.warn("One-way request {} is expired. Not executing.", String.valueOf(request));
                }
            }
        }

        if (futuresList != null) {
            for (ScheduledFuture<?> future : futuresList) {
                future.cancel(false);
            }
        }
    }

    @Override
    public void entryRemoved(String participantId) {
        // Cleanup not necessary
    }

    @Override
    public void handleOneWayRequest(final String providerParticipantId, final OneWayRequest request, long expiryDate) {
        ProviderContainer providerContainer = providerDirectory.get(providerParticipantId);
        if (providerContainer == null) {
            synchronized (requestQueueLock) {
                providerContainer = providerDirectory.get(providerParticipantId);
                if (providerContainer == null) {
                    logger.info("Provider participantId: {} not found, queuing one-way request message.",
                                providerParticipantId);
                    queueOneWayRequest(providerParticipantId,
                                       request,
                                       request.getMethodName(),
                                       ExpiryDate.fromAbsolute(expiryDate));
                    return;
                }
            }
        }
        handleOneWayRequest(providerContainer.getRequestCaller(), request);
    }

    private void handleOneWayRequest(RequestCaller requestCaller, OneWayRequest request) {
        try {
            requestInterpreter.invokeMethod(requestCaller, request);
        } catch (Exception e) {
            logger.error("Error while executing one-way request {}.", String.valueOf(request), e);
        }
    }

    // requires requestQueueLock
    private void queueOneWayRequest(final String providerParticipantId,
                                    OneWayRequest oneWayRequest,
                                    String methodName,
                                    ExpiryDate expiryDate) {

        if (!oneWayRequestQueue.containsKey(providerParticipantId)) {
            ConcurrentLinkedQueue<ContentWithExpiryDate<OneWayRequest>> newRequestList = new ConcurrentLinkedQueue<ContentWithExpiryDate<OneWayRequest>>();
            oneWayRequestQueue.putIfAbsent(providerParticipantId, newRequestList);
        }
        final ContentWithExpiryDate<OneWayRequest> requestItem = new ContentWithExpiryDate<OneWayRequest>(oneWayRequest,
                                                                                                          expiryDate);
        oneWayRequestQueue.get(providerParticipantId).add(requestItem);

        Runnable cleanupRunnable = new Runnable() {

            @Override
            public void run() {
                synchronized (requestQueueLock) {
                    ConcurrentLinkedQueue<ContentWithExpiryDate<OneWayRequest>> queue = oneWayRequestQueue.get(providerParticipantId);
                    if (queue != null && queue.remove(requestItem)) {
                        logger.warn("One-way request {} is expired. Not executing.", String.valueOf(oneWayRequest));
                        if (queue.isEmpty()) {
                            // cleanup
                            oneWayRequestQueue.remove(providerParticipantId);
                        }
                    }
                }
            }
        };
        scheduleCleanup(providerParticipantId, cleanupRunnable, expiryDate);
    }

    private void scheduleCleanup(final String providerParticipantId, Runnable cleanupRunnable, ExpiryDate expiryDate) {
        ScheduledFuture<?> cleanupSchedulerFuture = cleanupScheduler.schedule(cleanupRunnable,
                                                                              expiryDate.getRelativeTtl(),
                                                                              TimeUnit.MILLISECONDS);
        if (!cleanupSchedulerFuturesMap.containsKey(providerParticipantId)) {
            List<ScheduledFuture<?>> cleanupSchedulerFuturesList = new LinkedList<ScheduledFuture<?>>();
            cleanupSchedulerFuturesMap.put(providerParticipantId, cleanupSchedulerFuturesList);
        }
        cleanupSchedulerFuturesMap.get(providerParticipantId).add(cleanupSchedulerFuture);
    }

    @Override
    public void handleRequest(ProviderCallback<Reply> replyCallback,
                              String providerParticipantId,
                              Request request,
                              long expiryDate) {
        ProviderContainer providerContainer = providerDirectory.get(providerParticipantId);
        if (providerContainer == null) {
            synchronized (requestQueueLock) {
                providerContainer = providerDirectory.get(providerParticipantId);
                if (providerContainer == null) {
                    logger.info("Provider participantId: {} not found, queuing request message.",
                                providerParticipantId);
                    queueRequest(replyCallback, providerParticipantId, request, ExpiryDate.fromAbsolute(expiryDate));
                    return;
                }
            }
        }
        handleRequest(replyCallback, providerContainer.getRequestCaller(), request);
    }

    private void handleRequest(ProviderCallback<Reply> replyCallback, RequestCaller requestCaller, Request request) {
        logger.trace("Executing request {}", request.getRequestReplyId());
        requestInterpreter.execute(replyCallback, requestCaller, request);
    }

    // requires requestQueueLock
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

        Runnable cleanupRunnable = new Runnable() {
            @Override
            public void run() {
                synchronized (requestQueueLock) {
                    ConcurrentLinkedQueue<ContentWithExpiryDate<Request>> queue = requestQueue.get(providerParticipantId);
                    if (queue != null) {
                        queue.remove(requestItem);
                        if (queue.isEmpty()) {
                            // cleanup
                            oneWayRequestQueue.remove(providerParticipantId);
                        }
                    }
                    replyCallbacks.remove(requestItem.getContent());
                }
                Request request = requestItem.getContent();
                logger.warn("REQUEST expired and discarded: requestReplyId {}, providerParticipantId: {} request method: {}.",
                            request.getRequestReplyId(),
                            providerParticipantId,
                            request.getMethodName());

            }
        };
        scheduleCleanup(providerParticipantId, cleanupRunnable, expiryDate);
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

    @Override
    public void shutdown() {
        for (List<ScheduledFuture<?>> futuresList : cleanupSchedulerFuturesMap.values()) {
            if (futuresList != null) {
                for (ScheduledFuture<?> future : futuresList) {
                    future.cancel(false);
                }
            }
        }
        shuttingDown = true;
        synchronized (outstandingRequestFutures) {
            for (CompletableFuture<Reply> future : outstandingRequestFutures) {
                logger.debug("Shutting down. Interrupting task: {}", future.toString());
                future.cancel(true);
            }
        }
        providerDirectory.removeListener(this);
    }
}
