/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.standalonept.jee;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.concurrent.CompletableFuture;

import jakarta.ejb.Stateless;
import com.google.inject.Inject;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.messaging.MessagingQos;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.types.GlobalDiscoveryEntry;

@Stateless
@Path("/performance-tests")
@Produces(MediaType.APPLICATION_JSON)
public class ConsumerRestEndpoint {
    private static final Logger logger = LoggerFactory.getLogger(ConsumerRestEndpoint.class);

    private ServiceLocator serviceLocator;
    private String configuredDomain;
    final private String[] domains = new String[]{ "gfz.abcd" };
    final private String interfaceName = "go/wef/yg/sdf/ghtrqazs/OtmvqaZwertvbgfdwqBTG";
    final private Integer NUM_EXPECTED_FOUND_ENTRIES = 3;
    String[] configuredGbids;
    Integer calls;
    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

    private GlobalCapabilitiesDirectorySync proxy;

    private static void addStacktraceToResultString(Exception theException, StringBuffer result) {
        try (StringWriter writer = new StringWriter(); PrintWriter printWriter = new PrintWriter(writer)) {
            theException.printStackTrace(printWriter);
            result.append(writer.toString());
        } catch (IOException e) {
            String errorMsg = "Unable to add exception stacktrace to result string: " + e;
            result.append(errorMsg);
            logger.error(errorMsg);
        }
    }

    @Inject
    public ConsumerRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(240000); // 4 Min
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setRetryIntervalMs(5000); // 5 seconds
        messagingQos = new MessagingQos(240000); // 4 Minutes
        configuredDomain = System.getenv("PT_CONSUMER_DOMAIN");
        calls = Integer.parseInt(System.getenv("PT_NUMBER_OF_CALLS"));
        configuredGbids = System.getenv("PT_GBIDS").trim().split(",");
    }

    @GET
    @Path("/test")
    public String triggerPerformanceTest() {
        String domain = System.getenv("PT_PROVIDER_DOMAIN");
        StringBuffer logResult = new StringBuffer();

        try {
            if (calls <= 0) {
                throw new IllegalArgumentException("calls must be > 0");
            }
        } catch (Exception e) {
            logResult.append("throwing IllegalArgumentException \n");
            throw new IllegalArgumentException("number of calls should be greater than 0: got: " + calls);
        }

        try {
            String[] configuredGbids = System.getenv("PT_GBIDS").trim().split(",");
            logResult.append("triggerPerformanceTest method is called, => \n");
            CompletableFuture<GlobalCapabilitiesDirectorySync> future = serviceLocator.builder(GlobalCapabilitiesDirectorySync.class,
                                                                                               domain)
                                                                                      .useFuture()
                                                                                      .withDiscoveryQos(discoveryQos)
                                                                                      .withMessagingQos(messagingQos)
                                                                                      .withGbids(configuredGbids)
                                                                                      .build();
            // blocking call
            proxy = future.get();
            logResult.append("future.get() which gets built proxy called successfully, => \n");

            GlobalDiscoveryEntry[] lookupResult1 = null;
            int totalFoundEntries = 0;
            int executedLookups;
            for (executedLookups = 0; executedLookups < calls; ++executedLookups) {
                lookupResult1 = proxy.lookup(domains, interfaceName, configuredGbids); // expected 2
                totalFoundEntries += (lookupResult1 == null ? 0 : lookupResult1.length);
            }

            logResult.append("calling proxy.lookup(s) successfully, => \n");

            if (totalFoundEntries != executedLookups * NUM_EXPECTED_FOUND_ENTRIES) {
                logResult.append("throwing IllegalArgumentException \n");
                throw new Exception("number of expected found entries should be: "
                        + executedLookups * NUM_EXPECTED_FOUND_ENTRIES + ", got: " + totalFoundEntries);
            }
            logResult.append("PT RESULT success: JEE consumer ").append(configuredDomain).append(" -> ").append(domain);
        } catch (Exception e) {
            logger.error("Exception in triggerPerformanceTest: " + e);
            logResult.append("PT RESULT error: JEE consumer ")
                     .append(configuredDomain)
                     .append(" -> ")
                     .append(domain)
                     .append("\nException: ")
                     .append(e.toString());
            addStacktraceToResultString(e, logResult);
        }
        logResult.append("\n");
        return logResult.toString();
    }
}
