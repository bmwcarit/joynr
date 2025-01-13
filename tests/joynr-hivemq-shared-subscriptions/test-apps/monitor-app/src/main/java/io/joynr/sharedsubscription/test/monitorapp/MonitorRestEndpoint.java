/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.sharedsubscription.test.monitorapp;

import static java.lang.String.format;

import java.util.HashMap;
import java.util.Map;
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
    private PingServiceSync pingServiceClient;

    @Inject
    public MonitorRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
    }

    @GET
    @Path("/{numberOfPings}")
    public String triggerTest(@PathParam("numberOfPings") int numberOfPings) {
        Map<String, AtomicInteger> successCountByResult = new HashMap<>();
        int successCount = 0;
        int errorCount = 0;
        for (int count = 0; count < numberOfPings; count++) {
            try {
                String result = getService().ping();
                logger.info("Successfully pinged {}", result);
                successCount++;
                increaseCount(successCountByResult, result);
            } catch (RuntimeException e) {
                errorCount++;
                logger.error("Unable to call ping service.", e);
            }
        }
        return format("[SST] Triggered %d pings. %d were successful, %d failed.%n%s%n",
                      numberOfPings,
                      successCount,
                      errorCount,
                      dumpSuccessCountByResult(successCountByResult));
    }

    private String dumpSuccessCountByResult(Map<String, AtomicInteger> successCountByResult) {
        StringBuilder builder = new StringBuilder();
        for (Map.Entry<String, AtomicInteger> entry : successCountByResult.entrySet()) {
            builder.append("\t").append(entry.getKey()).append(": ").append(entry.getValue().get()).append("\n");
        }
        return builder.toString();
    }

    private void increaseCount(Map<String, AtomicInteger> counterMap, String result) {
        AtomicInteger counter = counterMap.get(result);
        if (counter == null) {
            counter = new AtomicInteger(0);
            counterMap.put(result, counter);
        }
        counter.incrementAndGet();
    }

    private PingServiceSync getService() {
        if (pingServiceClient == null) {
            pingServiceClient = serviceLocator.get(PingServiceSync.class,
                                                   "io.joynr.sharedsubscriptions.test.clusteredapp",
                                                   5000);
        }
        return pingServiceClient;
    }

}
