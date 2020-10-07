/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import javax.inject.Inject;
import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.exceptions.ApplicationException;
import joynr.io.joynr.sharedsubscriptions.test.PingServiceAsync;

@Path("/test")
@Produces(MediaType.APPLICATION_JSON)
@Consumes(MediaType.APPLICATION_JSON)
public class MonitorRestEndpoint {

    private static final Logger logger = LoggerFactory.getLogger(MonitorRestEndpoint.class);

    private ServiceLocator serviceLocator;
    private PingServiceAsync pingServiceClient;

    @Inject
    public MonitorRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
    }

    @GET
    @Path("/{numberOfPings}")
    public String triggerTest(@PathParam("numberOfPings") int numberOfPings) {
        final int minRequiredPings = 200;
        final int triggerUnsubscribeOfSmallProviderPings = 200;
        final int onlyForLargeProviderPings = 100;

        if (numberOfPings < minRequiredPings) {
            return format("Backpressure test needs more than %d pings", minRequiredPings);
        }

        Map<String, AtomicInteger> initialPhaseCountByProvider = new ConcurrentHashMap<>();
        Map<String, AtomicInteger> backpressurePhaseCountByProvider = new ConcurrentHashMap<>();
        Map<String, AtomicInteger> finalPhaseCountByProvider = new ConcurrentHashMap<>();

        AtomicInteger totalSuccessCount = new AtomicInteger(0);
        AtomicInteger totalErrorCount = new AtomicInteger(0);
        List<Future<String>> allFutures = new LinkedList<>();

        // both clustered providers should get pings until the small one is overloaded and unsubscribes
        bulkSendAsyncRequests(minRequiredPings,
                              initialPhaseCountByProvider,
                              totalSuccessCount,
                              totalErrorCount,
                              allFutures);

        // only the large provider should get pings
        bulkSendAsyncRequests(onlyForLargeProviderPings,
                              backpressurePhaseCountByProvider,
                              totalSuccessCount,
                              totalErrorCount,
                              allFutures);

        try {
            Thread.sleep(10000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        // again both clustered providers should be available, i.e.
        // the small provider should have registered again
        final int remainingPings = numberOfPings - triggerUnsubscribeOfSmallProviderPings - onlyForLargeProviderPings;
        bulkSendAsyncRequests(remainingPings,
                              finalPhaseCountByProvider,
                              totalSuccessCount,
                              totalErrorCount,
                              allFutures);

        // wait for all requests to complete
        for (Future<String> future : allFutures) {
            try {
                future.get();
            } catch (JoynrWaitExpiredException e) {
                e.printStackTrace();
            } catch (JoynrRuntimeException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (ApplicationException e) {
                e.printStackTrace();
            }
        }

        return evaluateTest(numberOfPings,
                            totalSuccessCount,
                            totalErrorCount,
                            initialPhaseCountByProvider,
                            backpressurePhaseCountByProvider,
                            finalPhaseCountByProvider);
    }

    private String evaluateTest(int numberOfPings,
                                AtomicInteger totalSuccessCount,
                                AtomicInteger totalErrorCount,
                                Map<String, AtomicInteger> initialPhaseCountByProvider,
                                Map<String, AtomicInteger> backpressurePhaseCountByProvider,
                                Map<String, AtomicInteger> finalPhaseCountByProvider) {
        boolean success = true;
        String testEvalString = "";

        //rule all pings successful
        if (numberOfPings > totalSuccessCount.get()) {
            success = false;
        }

        //rule no errors
        if (totalErrorCount.get() > 0) {
            success = false;
        }

        // rule small provider should respond in initial phase
        int smallInstanceResponses = getSmallInstanceResponseCountFromMap(initialPhaseCountByProvider);
        if (smallInstanceResponses == 0) {
            success = false;
        }

        // rule no responses from small provider during backpressure phase
        smallInstanceResponses = getSmallInstanceResponseCountFromMap(backpressurePhaseCountByProvider);
        if (smallInstanceResponses > 0) {
            success = false;
        }

        // rule small provider should respond again in final phase
        smallInstanceResponses = getSmallInstanceResponseCountFromMap(finalPhaseCountByProvider);
        if (smallInstanceResponses == 0) {
            success = false;
        }

        // create final output
        testEvalString = format("%s%nTriggered %d pings. %d were successful, %d failed.%n",
                                success ? "SUCCESS" : "FAILURE",
                                numberOfPings,
                                totalSuccessCount.get(),
                                totalErrorCount.get());

        testEvalString += format("Initial phase:%n%s%n", dumpResponseCounterMap(initialPhaseCountByProvider));

        testEvalString += format("Backpressure phase:%n%s%n", dumpResponseCounterMap(backpressurePhaseCountByProvider));

        testEvalString += format("Final phase:%n%s", dumpResponseCounterMap(finalPhaseCountByProvider));

        return testEvalString;
    }

    private Integer getSmallInstanceResponseCountFromMap(Map<String, AtomicInteger> finalPhaseCountByProvider) {
        return finalPhaseCountByProvider.entrySet()
                                        .stream()
                                        .filter(e -> e.getKey().startsWith("small"))
                                        .map(e -> e.getValue().get())
                                        .findFirst()
                                        .orElse(0);
    }

    private void bulkSendAsyncRequests(final int numberOfRequests,
                                       Map<String, AtomicInteger> responseCounterMap,
                                       AtomicInteger totalSuccessCount,
                                       AtomicInteger totalErrorCount,
                                       List<Future<String>> allFutures) {
        for (int i = 0; i < numberOfRequests; i++) {
            Future<String> result = getService().ping(new Callback<String>() {

                @Override
                public void onFailure(JoynrRuntimeException arg0) {
                    totalErrorCount.incrementAndGet();
                    logger.error("Unable to call ping service.", arg0);
                }

                @Override
                public void onSuccess(String arg0) {
                    logger.info("Successfully pinged {}", arg0);
                    totalSuccessCount.incrementAndGet();
                    increaseCount(responseCounterMap, arg0);
                }

            });
            allFutures.add(result);
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

    private PingServiceAsync getService() {
        if (pingServiceClient == null) {
            final int messagesTtlMs = 70000;
            pingServiceClient = serviceLocator.get(PingServiceAsync.class,
                                                   "io.joynr.backpressure.test.provider",
                                                   messagesTtlMs);
        }
        return pingServiceClient;
    }

}
