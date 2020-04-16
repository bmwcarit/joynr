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

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.Directory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;

/**
 * Queue to store replyCallers and remove them if the round-trip TTL of the corresponding request expires.
 *
 */
@Singleton
public class ReplyCallerDirectory extends Directory<ReplyCaller> implements ShutdownListener {

    private boolean shutdown = false;
    private static final Logger logger = LoggerFactory.getLogger(ReplyCallerDirectory.class);

    private ScheduledExecutorService cleanupScheduler;

    private ConcurrentMap<String, ScheduledFuture<?>> cleanupSchedulerFuturesMap = null;

    @Inject
    public ReplyCallerDirectory(@Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                ShutdownNotifier shutdownNotifier) {
        this.cleanupScheduler = cleanupScheduler;
        this.cleanupSchedulerFuturesMap = new ConcurrentHashMap<>();
        shutdownNotifier.registerForShutdown(this);
    }

    public void addReplyCaller(final String requestReplyId,
                               final ReplyCaller replyCaller,
                               final ExpiryDate roundTripTtlExpirationDate) {
        logger.trace("AddReplyCaller: requestReplyId: {}, expiryDate: {}", requestReplyId, roundTripTtlExpirationDate);
        if (super.contains(requestReplyId)) {
            logger.error("RequestReplyId should not be replicated: {}", requestReplyId);
        } else {
            super.add(requestReplyId, replyCaller);
            try {
                ScheduledFuture<?> cleanupSchedulerFuture = cleanupScheduler.schedule(new Runnable() {
                    @Override
                    public void run() {
                        removeExpiredReplyCaller(requestReplyId);
                    }
                }, roundTripTtlExpirationDate.getRelativeTtl(), TimeUnit.MILLISECONDS);
                cleanupSchedulerFuturesMap.put(requestReplyId, cleanupSchedulerFuture);
            } catch (RejectedExecutionException e) {
                if (shutdown) {
                    throw new JoynrShutdownException("shutdown in ReplyCallerDirectory");
                }
                throw new JoynrRuntimeException(e);
            }
        }

    }

    @Override
    public ReplyCaller remove(String id) {
        ReplyCaller replyCaller = super.remove(id);
        ScheduledFuture<?> future = cleanupSchedulerFuturesMap.remove(id);
        if (future != null) {
            future.cancel(false);
        }
        return replyCaller;
    }

    private void removeExpiredReplyCaller(String requestReplyId) {
        ReplyCaller outstandingReplyCaller = remove(requestReplyId);
        if (outstandingReplyCaller == null) {
            // this happens, when a reply was already received and the replyCaller has been removed.
            return;
        }
        logger.debug("Replycaller with requestReplyId {} was removed because TTL expired", requestReplyId);

        // notify the caller that the request has expired now
        outstandingReplyCaller.error(new JoynrTimeoutException(System.currentTimeMillis()));

    }

    @Override
    public void shutdown() {
        for (ScheduledFuture<?> cleanupSchedulerFuture : cleanupSchedulerFuturesMap.values()) {
            cleanupSchedulerFuture.cancel(false);
        }

        shutdown = true;
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
