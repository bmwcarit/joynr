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
package io.joynr.capabilities;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.runtime.JoynrInjectionConstants;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;

import joynr.types.DiscoveryEntry;

/**
 * Use this class to periodically check a {@link DiscoveryEntryStore} for expired entries and clean them out by calling
 * the provided {@link CleanupAction clean-up action callback}.
 * <p>
 * This implementation uses the {@link ScheduledExecutorService} configured under {@link JoynrInjectionConstants#JOYNR_SCHEDULER_CLEANUP}
 * in order to schedule the cleanup tasks. The interval in which discovery entries are cleaned up can be configured
 * using the {@link io.joynr.common.JoynrPropertiesModule joynr property} {@link #DISCOVERY_ENTRY_CACHE_CLEANUP_INTERVAL},
 * which allows specifying the interval in minutes. The default value is set in the <code>defaultMessaging.properties</code>
 * file.
 * </p>
 * <p>
 *     Call the {@link #scheduleCleanUpForCaches(CleanupAction, DiscoveryEntryStore[])} method with the stores to be cleaned up at regular
 *     intervals to start the process.
 * </p>
 */
@Singleton
public class ExpiredDiscoveryEntryCacheCleaner implements ShutdownListener {

    private static final Logger logger = LoggerFactory.getLogger(ExpiredDiscoveryEntryCacheCleaner.class);

    public static final String DISCOVERY_ENTRY_CACHE_CLEANUP_INTERVAL = "joynr.cc.discovery.entry.cache.cleanup.interval";
    private ScheduledFuture<?> scheduledFuture;

    /**
     * Implementations of this are registered with {@link #scheduleCleanUpForCaches(CleanupAction, DiscoveryEntryStore...)} to be
     * called for each {@link DiscoveryEntry} found to be expired in the given cache.
     */
    public static interface CleanupAction {
        void cleanup(Set<DiscoveryEntry> expiredDiscoveryEntries);
    }

    private ScheduledExecutorService scheduledExecutorService;
    private int cacheCleanupIntervalInMinutes;
    private final ShutdownNotifier shutdownNotifier;

    @Inject
    public ExpiredDiscoveryEntryCacheCleaner(@Named(JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService scheduledExecutorService,
                                             @Named(DISCOVERY_ENTRY_CACHE_CLEANUP_INTERVAL) int cacheCleanupIntervalInMinutes,
                                             ShutdownNotifier shutdownNotifier) {
        this.scheduledExecutorService = scheduledExecutorService;
        this.cacheCleanupIntervalInMinutes = cacheCleanupIntervalInMinutes;
        this.shutdownNotifier = shutdownNotifier;
        shutdownNotifier.registerForShutdown(this);
    }

    public void shutdown() {
        logger.info("shutdown invoked");
        scheduledFuture.cancel(true);
        logger.info("shutdown finished");
    }

    public void scheduleCleanUpForCaches(final CleanupAction cleanupAction, final DiscoveryEntryStore<?>... caches) {
        scheduledFuture = scheduledExecutorService.scheduleAtFixedRate(new Runnable() {
            @Override
            public void run() {
                try {
                    doCleanupFor(cleanupAction, caches);
                } catch (Exception e) {
                    logger.error("Problem encountered while cleaning up expired discovery entries on cache {}",
                                 caches,
                                 e);
                }
            }
        }, cacheCleanupIntervalInMinutes, cacheCleanupIntervalInMinutes, TimeUnit.MINUTES);
    }

    private void doCleanupFor(CleanupAction cleanupAction, DiscoveryEntryStore<?>... caches) {
        Set<DiscoveryEntry> expiredDiscoveryEntries = new HashSet<>();
        long now = System.currentTimeMillis();
        for (DiscoveryEntryStore<? extends DiscoveryEntry> cache : caches) {
            for (DiscoveryEntry discoveryEntry : cache.getAllDiscoveryEntries()) {
                if (discoveryEntry.getExpiryDateMs() < now) {
                    expiredDiscoveryEntries.add(discoveryEntry);
                }
            }
        }
        logger.debug("The following expired participant IDs will be cleaned from the caches {}: {}",
                     Arrays.toString(caches),
                     expiredDiscoveryEntries);
        cleanupAction.cleanup(expiredDiscoveryEntries);
    }
}
