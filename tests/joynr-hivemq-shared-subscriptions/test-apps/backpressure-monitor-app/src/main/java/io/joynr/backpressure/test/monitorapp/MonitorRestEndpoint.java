/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
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
package io.joynr.backpressure.test.monitorapp;

import static java.lang.String.format;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import jakarta.inject.Inject;
import jakarta.ws.rs.Consumes;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.PathParam;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.io.joynr.sharedsubscriptions.test.PingServiceSync;

@Path("/test")
@Produces(MediaType.APPLICATION_JSON)
@Consumes(MediaType.APPLICATION_JSON)
public class MonitorRestEndpoint {

    private static final Logger logger = LoggerFactory.getLogger(MonitorRestEndpoint.class);

    private ServiceLocator serviceLocator;

    @Inject
    public MonitorRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
    }

    @GET
    @Path("/{numberOfWorkers}/{pingsPerWorker}")
    public String triggerTest(@PathParam("numberOfWorkers") int numberOfWorkers,
                              @PathParam("pingsPerWorker") int pingsPerWorker) {
        final int minRequiredPings = 400;
        final int numberOfPings = numberOfWorkers * pingsPerWorker;
        if (numberOfWorkers < 1) {
            return format("Number of workers must be at least 1, current value: %d", numberOfWorkers);
        }
        if (numberOfPings < minRequiredPings) {
            return format("Backpressure test needs more than %d pings", minRequiredPings);
        }

        Map<String, AtomicInteger> backpressureNodeResponseCounter = new ConcurrentHashMap<>();
        AtomicInteger successCount = new AtomicInteger(0);
        AtomicInteger errorCount = new AtomicInteger(0);

        ExecutorService pool = Executors.newFixedThreadPool(numberOfWorkers);
        for (int i = 0; i < numberOfWorkers; ++i) {
            Runnable task = () -> {
                bulkSendRequests(pingsPerWorker, successCount, errorCount, backpressureNodeResponseCounter);
            };
            pool.submit(task);
        }
        pool.shutdown();
        try {
            boolean terminationResult = pool.awaitTermination(10 * 60, TimeUnit.SECONDS);
            logger.info("Pool termination result: {}", terminationResult);
        } catch (InterruptedException e) {
            logger.warn("awaitTermination timeout.");
        }

        return format("[BPT] Triggered %d pings. %d were successful, %d failed.%n%s%n",
                      numberOfPings,
                      successCount.get(),
                      errorCount.get(),
                      dumpResponseCounterMap(backpressureNodeResponseCounter));
    }

    private void bulkSendRequests(final int numberOfRequests,
                                  AtomicInteger successCount,
                                  AtomicInteger errorCount,
                                  Map<String, AtomicInteger> responseCounterMap) {
        try {
            PingServiceSync pingServiceSync = getService();
            for (int i = 0; i < numberOfRequests; i++) {
                try {
                    String result = pingServiceSync.ping();
                    successCount.incrementAndGet();
                    increaseCount(responseCounterMap, result);
                    logger.info("Successfully pinged: {}", result);
                } catch (RuntimeException e) {
                    errorCount.incrementAndGet();
                    logger.error("Unable to call ping service.", e);
                }
            }
        } catch (java.lang.Exception e) {
            logger.error("Unable to build ping service proxy.", e);
        }
    }

    private String dumpResponseCounterMap(Map<String, AtomicInteger> responseCounterMap) {
        StringBuilder builder = new StringBuilder();
        for (Map.Entry<String, AtomicInteger> entry : responseCounterMap.entrySet()) {
            builder.append("\t").append(entry.getKey()).append(": ").append(entry.getValue().get()).append("\n");
        }
        return builder.toString();
    }

    private void increaseCount(Map<String, AtomicInteger> responseCounterMap, String response) {
        synchronized (responseCounterMap) {
            AtomicInteger counter = responseCounterMap.get(response);
            if (counter == null) {
                counter = new AtomicInteger(0);
                responseCounterMap.put(response, counter);
            }
            counter.incrementAndGet();
        }
    }

    private PingServiceSync getService() {
        final int messagesTtlMs = 5 * 60000;
        return serviceLocator.builder(PingServiceSync.class, "io.joynr.backpressure.test.provider")
                             .withTtl(messagesTtlMs)
                             .build();
    }

}
