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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrTimeoutException;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Queue to store replyCallers and remove them if the round-trip TTL of the corresponding request expires.
 * 
 */
@Singleton
public class ReplyCallerDirectory {
    private ConcurrentHashMap<String, ContentWithExpiryDate<ReplyCaller>> outstandingReplyCallers = new ConcurrentHashMap<String, ContentWithExpiryDate<ReplyCaller>>();

    private boolean shutdown = false;
    private static final Logger logger = LoggerFactory.getLogger(ReplyCallerDirectory.class);

    private ScheduledExecutorService cleanupScheduler;

    @Inject
    public ReplyCallerDirectory(@Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler) {
        this.cleanupScheduler = cleanupScheduler;
    }

    public void putReplyCaller(final String requestReplyId,
                               final ReplyCaller replyCaller,
                               final ExpiryDate roundTripTtlExpirationDate) {
        logger.trace("putReplyCaller: " + requestReplyId + " expiryDate: " + roundTripTtlExpirationDate);
        outstandingReplyCallers.putIfAbsent(requestReplyId,
                                            new ContentWithExpiryDate<ReplyCaller>(replyCaller,
                                                                                   roundTripTtlExpirationDate));
        try {
            cleanupScheduler.schedule(new Runnable() {
                public void run() {
                    removeExpiredReplyCaller(requestReplyId);
                }
            }, roundTripTtlExpirationDate.getRelativeTtl(), TimeUnit.MILLISECONDS);
        } catch (RejectedExecutionException e) {
            if (shutdown) {
                throw new JoynrShutdownException("shutdown in ReplyCallerDirectory");
            }
            throw new JoynrRuntimeException(e);
        }

    }

    @CheckForNull
    public ReplyCaller getAndRemoveReplyCaller(String requestReplyId) {
        logger.trace("getAndRemoveReplyCaller: {}", requestReplyId);
        ContentWithExpiryDate<ReplyCaller> remove = outstandingReplyCallers.remove(requestReplyId);
        if (remove == null) {
            logger.trace("getAndRemoveReplyCaller: {} not found", requestReplyId);
            return null;
        }
        return remove.getContent();
    }

    public boolean containsReplyCallerFor(String replyId) {
        return outstandingReplyCallers.containsKey(replyId);
    }

    private void removeExpiredReplyCaller(String requestReplyId) {
        ContentWithExpiryDate<ReplyCaller> remove = outstandingReplyCallers.remove(requestReplyId);
        if (remove == null) {
            // this happens, when a reply was already received and the replyCaller has been removed.
            return;
        }
        ReplyCaller outstandingReplyCaller = remove.getContent();
        logger.debug("Replycaller with requestReplyId " + requestReplyId + " was removed because TTL expired");

        logger.trace("Removing replyCaller requestReplyId: {} from queue because it is expired",
                     outstandingReplyCaller.getRequestReplyId());

        // notify the caller that the request has expired now
        outstandingReplyCaller.error(new JoynrTimeoutException(System.currentTimeMillis()));

    }

    public void shutdown() {
        // outstandingReplyCaller.values();
        // for (ContentWithExpiryDate<ReplyCaller> wrapper : outstandingReplyCaller.values()) {
        // ReplyCaller replyCaller = wrapper.getContent();
        // replyCaller.shutdown();
        //
        // }
        shutdown = true;
    }

    public boolean isEmpty() {
        if (outstandingReplyCallers == null) {
            return true;
        }

        return outstandingReplyCallers.isEmpty();
    }
}
