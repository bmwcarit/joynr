package io.joynr.dispatching.rpc;

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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.Directory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;

import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

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
public class ReplyCallerDirectory extends Directory<ReplyCaller> implements ShutdownListener {

    private boolean shutdown = false;
    private static final Logger logger = LoggerFactory.getLogger(ReplyCallerDirectory.class);

    private ScheduledExecutorService cleanupScheduler;

    @Inject
    public ReplyCallerDirectory(@Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                ShutdownNotifier shutdownNotifier) {
        this.cleanupScheduler = cleanupScheduler;
        shutdownNotifier.registerForShutdown(this);
    }

    public void addReplyCaller(final String requestReplyId,
                               final ReplyCaller replyCaller,
                               final ExpiryDate roundTripTtlExpirationDate) {
        logger.trace("putReplyCaller: " + requestReplyId + " expiryDate: " + roundTripTtlExpirationDate);
        super.add(requestReplyId, replyCaller);

        try {
            cleanupScheduler.schedule(new Runnable() {
                @Override
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

    private void removeExpiredReplyCaller(String requestReplyId) {
        ReplyCaller outstandingReplyCaller = remove(requestReplyId);
        if (outstandingReplyCaller == null) {
            // this happens, when a reply was already received and the replyCaller has been removed.
            return;
        }
        logger.debug("Replycaller with requestReplyId " + requestReplyId + " was removed because TTL expired");

        // notify the caller that the request has expired now
        outstandingReplyCaller.error(new JoynrTimeoutException(System.currentTimeMillis()));

    }

    @Override
    public void shutdown() {
        shutdown = true;
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
