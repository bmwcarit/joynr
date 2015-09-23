package io.joynr.dispatching;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.proxy.Callback;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import joynr.OneWay;
import joynr.Reply;
import joynr.Request;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Default implementation of the Dispatcher interface.
 */
@Singleton
public class RequestReplyDispatcherImpl implements RequestReplyDispatcher, CallerDirectoryListener<RequestCaller> {

    private static final Logger logger = LoggerFactory.getLogger(RequestReplyDispatcherImpl.class);

    private Map<String, PayloadListener<?>> oneWayRecipients = Maps.newHashMap();
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>> requestQueue = new ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>>();
    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<OneWay>>> oneWayRequestQueue = new ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<OneWay>>>();
    private ConcurrentHashMap<Request, Callback<Reply>> replyCallbacks = new ConcurrentHashMap<Request, Callback<Reply>>();

    private ReplyCallerDirectory replyCallerDirectory;
    private RequestCallerDirectory requestCallerDirectory;
    private RequestInterpreter requestInterpreter;

    private ScheduledExecutorService cleanupScheduler;

    @Inject
    public RequestReplyDispatcherImpl(ReplyCallerDirectory replyCallerDirectory,
                                      RequestInterpreter requestInterpreter,
                                      RequestCallerDirectory requestCallerDirectory,
                                      @Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler) {
        this.replyCallerDirectory = replyCallerDirectory;
        this.requestInterpreter = requestInterpreter;
        this.requestCallerDirectory = requestCallerDirectory;
        requestCallerDirectory.addListener(this);
        this.cleanupScheduler = cleanupScheduler;
    }

    @Override
    public void addOneWayRecipient(final String participantId, PayloadListener<?> listener) {
        synchronized (oneWayRecipients) {
            oneWayRecipients.put(participantId, listener);
        }

        ConcurrentLinkedQueue<ContentWithExpiryDate<OneWay>> oneWayRequestList = oneWayRequestQueue.remove(participantId);
        if (oneWayRequestList != null) {
            for (ContentWithExpiryDate<OneWay> oneWayRequestItem : oneWayRequestList) {
                if (!oneWayRequestItem.isExpired()) {
                    handleOneWayRequest(listener, oneWayRequestItem.getContent());
                }
            }
        }
    }

    @Override
    public void callerAdded(String participantId, RequestCaller requestCaller) {
        ConcurrentLinkedQueue<ContentWithExpiryDate<Request>> requestList = requestQueue.remove(participantId);
        if (requestList != null) {
            for (ContentWithExpiryDate<Request> requestItem : requestList) {
                if (!requestItem.isExpired()) {
                    Request request = requestItem.getContent();
                    handleRequest(replyCallbacks.remove(request), requestCaller, request);
                }
            }
        }
    }

    @Override
    public void callerRemoved(String participantId) {
        //TODO cleanup requestQueue?
    }

    @Override
    public void removeListener(final String participantId) {
        synchronized (oneWayRecipients) {
            oneWayRecipients.remove(participantId);
        }
    }

    @Override
    public void handleOneWayRequest(String providerParticipantId, OneWay requestPayload, long expiryDate) {
        synchronized (oneWayRecipients) {
            final PayloadListener<?> listener = oneWayRecipients.get(providerParticipantId);
            if (listener != null) {
                handleOneWayRequest(listener, requestPayload);
            } else {
                queueOneWayRequest(providerParticipantId, requestPayload, ExpiryDate.fromAbsolute(expiryDate));
            }
        }
    }

    @SuppressWarnings("unchecked")
    private void handleOneWayRequest(final PayloadListener listener, final OneWay requestPayload) {
        listener.receive(requestPayload.getPayload());
    }

    @Override
    public void handleRequest(Callback<Reply> replyCallback,
                              String providerParticipant,
                              Request request,
                              long expiryDate) {
        if (requestCallerDirectory.containsCaller(providerParticipant)) {
            handleRequest(replyCallback, requestCallerDirectory.getCaller(providerParticipant), request);
        } else {
            queueRequest(replyCallback, providerParticipant, request, ExpiryDate.fromAbsolute(expiryDate));
            logger.info("No requestCaller found for participantId: {} queuing request message.", providerParticipant);
        }
    }

    private void handleRequest(Callback<Reply> replyCallback, RequestCaller requestCaller, Request request) {
        // TODO shall be moved to request manager and not handled by dispatcher
        logger.debug("executing request {}", request.getRequestReplyId());
        requestInterpreter.execute(replyCallback, requestCaller, request);
    }

    @Override
    public void handleReply(final Reply reply) {
        final ReplyCaller callBack = replyCallerDirectory.removeCaller(reply.getRequestReplyId());
        if (callBack == null) {
            logger.warn("No reply caller found for id: " + reply.getRequestReplyId());
            return;
        }

        callBack.messageCallBack(reply);
    }

    @Override
    public void handleError(Request request, Throwable error) {
        String requestReplyId = request.getRequestReplyId();
        if (requestReplyId != null) {
            ReplyCaller replyCaller = replyCallerDirectory.removeCaller(requestReplyId);
            if (replyCaller != null) {
                replyCaller.error(error);
            }
        }
    }

    @Override
    public void shutdown() {
        logger.info("SHUTTING DOWN RequestReplyDispatcher");
        requestCallerDirectory.removeListener(this);
        replyCallerDirectory.shutdown();
    }

    private void queueRequest(final Callback<Reply> replyCallback,
                              final String providerParticipantId,
                              Request request,
                              ExpiryDate incomingTtlExpirationDate_ms) {

        if (!requestQueue.containsKey(providerParticipantId)) {
            ConcurrentLinkedQueue<ContentWithExpiryDate<Request>> newRequestList = new ConcurrentLinkedQueue<ContentWithExpiryDate<Request>>();
            requestQueue.putIfAbsent(providerParticipantId, newRequestList);
        }
        final ContentWithExpiryDate<Request> requestItem = new ContentWithExpiryDate<Request>(request,
                                                                                              incomingTtlExpirationDate_ms);
        requestQueue.get(providerParticipantId).add(requestItem);
        replyCallbacks.put(request, replyCallback);
        cleanupScheduler.schedule(new Runnable() {

            @Override
            public void run() {
                requestQueue.get(providerParticipantId).remove(requestItem);
                replyCallbacks.remove(requestItem.getContent());
                Request request = requestItem.getContent();
                logger.warn("TTL DISCARD. providerParticipantId: {} request method: {} because it has expired. ",
                            new String[]{ providerParticipantId, request.getMethodName() });

            }
        }, incomingTtlExpirationDate_ms.getRelativeTtl(), TimeUnit.MILLISECONDS);
    }

    private void queueOneWayRequest(final String providerParticipantId,
                                    OneWay request,
                                    ExpiryDate incomingTtlExpirationDate_ms) {

        if (!oneWayRequestQueue.containsKey(providerParticipantId)) {
            ConcurrentLinkedQueue<ContentWithExpiryDate<OneWay>> newOneWayRequestList = new ConcurrentLinkedQueue<ContentWithExpiryDate<OneWay>>();
            oneWayRequestQueue.putIfAbsent(providerParticipantId, newOneWayRequestList);
        }
        final ContentWithExpiryDate<OneWay> oneWayRequestItem = new ContentWithExpiryDate<OneWay>(request,
                                                                                                  incomingTtlExpirationDate_ms);
        oneWayRequestQueue.get(providerParticipantId).add(oneWayRequestItem);
        cleanupScheduler.schedule(new Runnable() {

            @Override
            public void run() {
                oneWayRequestQueue.get(providerParticipantId).remove(oneWayRequestItem);
                logger.warn("TTL DISCARD. providerParticipantId: {} one way request with payload: {} because it has expired. ",
                            new String[]{ providerParticipantId, oneWayRequestItem.getContent().toString() });

            }
        },
                                  incomingTtlExpirationDate_ms.getRelativeTtl(),
                                  TimeUnit.MILLISECONDS);
    }

}
