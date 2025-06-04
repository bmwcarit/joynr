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
package io.joynr.dispatching.rpc;

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;

import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.Directory;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.messaging.tracking.MessageTrackerForGracefulShutdown;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;

/**
 * Queue to store replyCallers and remove them if the round-trip TTL of the corresponding request expires.
 */
@Singleton
public class ReplyCallerDirectory extends Directory<ReplyCaller> implements ShutdownListener {

    private final static long CLEANUP_TASK_INTERVAL_MS = 5000L;
    private AtomicBoolean shutdown = new AtomicBoolean(false);
    private static final Logger logger = LoggerFactory.getLogger(ReplyCallerDirectory.class);

    private ScheduledExecutorService cleanupScheduler;

    private MessageTrackerForGracefulShutdown messageTracker;

    private ScheduledFuture<?> cleanupTaskFuture;

    private Map<String, ExpiryDate> expiryDateMap;

    @Inject
    public ReplyCallerDirectory(@Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                ShutdownNotifier shutdownNotifier,
                                MessageTrackerForGracefulShutdown messageTracker) {
        this.cleanupScheduler = cleanupScheduler;
        this.expiryDateMap = new ConcurrentHashMap<>();
        shutdownNotifier.registerForShutdown(this);
        this.messageTracker = messageTracker;
        cleanupTaskFuture = this.cleanupScheduler.scheduleAtFixedRate(this::removeExpiredReplyCallers,
                                                                      CLEANUP_TASK_INTERVAL_MS,
                                                                      CLEANUP_TASK_INTERVAL_MS,
                                                                      TimeUnit.MILLISECONDS);
    }

    public void addReplyCaller(final String requestReplyId,
                               final ReplyCaller replyCaller,
                               final ExpiryDate roundTripTtlExpirationDate) {
        logger.trace("AddReplyCaller: requestReplyId: {}, expiryDate: {}", requestReplyId, roundTripTtlExpirationDate);
        if (shutdown.get()) {
            throw new JoynrShutdownException("shutdown in ReplyCallerDirectory");
        }
        if (super.get(requestReplyId) != null) {
            logger.error("RequestReplyId should not be replicated: {}", requestReplyId);
        } else {
            super.add(requestReplyId, replyCaller);
            expiryDateMap.put(requestReplyId, roundTripTtlExpirationDate);
        }
    }

    @Override
    public ReplyCaller remove(String id) {
        ReplyCaller replyCaller;
        replyCaller = super.remove(id);
        expiryDateMap.remove(id);
        return replyCaller;
    }

    private void removeExpiredReplyCallers() {
        ArrayList<String> expiredReplyCallers = new ArrayList<>();
        super.forEach((requestReplyId, replyCaller) -> {
            ExpiryDate expiryDate = expiryDateMap.get(requestReplyId);
            if (expiryDate == null) {
                logger.error("Could not find expiry date entry for requestReplyId: {}", requestReplyId);
                return;
            }
            if (expiryDate.getValue() - System.currentTimeMillis() < 0) {
                expiredReplyCallers.add(requestReplyId);
            }
        });
        for (String requestReplyId : expiredReplyCallers) {
            removeExpiredReplyCaller(requestReplyId);
        }
    }

    private void removeExpiredReplyCaller(String requestReplyId) {
        messageTracker.unregisterAfterReplyCallerExpired(requestReplyId);
        ReplyCaller outstandingReplyCaller = remove(requestReplyId);
        if (outstandingReplyCaller == null) {
            // this happens, when a reply was already received and the replyCaller has been removed.
            return;
        }
        logger.debug("Replycaller with requestReplyId {} was removed because TTL expired", requestReplyId);

        // notify the caller that the request has expired now
        outstandingReplyCaller.error(new JoynrTimeoutException(System.currentTimeMillis(), requestReplyId));
    }

    @Override
    public void shutdown() {
        cleanupTaskFuture.cancel(false);
        shutdown.set(true);
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
