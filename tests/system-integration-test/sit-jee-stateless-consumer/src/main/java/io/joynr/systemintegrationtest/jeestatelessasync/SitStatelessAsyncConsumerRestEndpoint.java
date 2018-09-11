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
package io.joynr.systemintegrationtest.jeestatelessasync;

import java.util.concurrent.CountDownLatch;

import javax.inject.Inject;
import javax.ws.rs.DefaultValue;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.messaging.MessagingQos;
import io.joynr.systemintegrationtest.jee.Util;
import joynr.test.SystemIntegrationTestStatelessAsync;

/**
 * Exposes a REST endpoint with which the joynr stateless async system integration tests can be triggered.
 */
@Path("/stateless-consumer")
@Produces(MediaType.APPLICATION_JSON)
public class SitStatelessAsyncConsumerRestEndpoint {
    private static final Logger logger = LoggerFactory.getLogger(SitStatelessAsyncConsumerRestEndpoint.class);
    private static final String SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String USE_CASE = "sit-jee-stateless-async";

    private ServiceLocator serviceLocator;
    private StatelessResultQueue resultQueue;

    @Inject
    public SitStatelessAsyncConsumerRestEndpoint(ServiceLocator serviceLocator, StatelessResultQueue resultQueue) {
        this.serviceLocator = serviceLocator;
        this.resultQueue = resultQueue;
    }

    @GET
    @Path("/ping")
    public String ping() {
        logger.info("ping called");
        return "OK";
    }

    @GET
    @Path("/result")
    public String waitForResult(@DefaultValue("60000") @QueryParam("timeoutMs") int timeoutMs) {
        logger.info("waitForResult called");
        try {
            return resultQueue.getResult(timeoutMs);
        } catch (JoynrWaitExpiredException | InterruptedException e) {
            String errorMsg = "SIT RESULT error: stateless async JEE consumer -> " + e.toString();
            logger.error(errorMsg);
            return errorMsg;
        }
    }

    @GET
    @Path("/test")
    public String triggerTests() {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();
        callProducer(SIT_DOMAIN_PREFIX + ".node", result);
        return result.toString();
    }

    private void callProducer(String domain, StringBuffer result) {
        try {
            DiscoveryQos discoveryQos = new DiscoveryQos();
            discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes

            SystemIntegrationTestStatelessAsync proxy = serviceLocator.builder(SystemIntegrationTestStatelessAsync.class,
                                                                               domain)
                                                                      .withUseCase(USE_CASE)
                                                                      .withDicoveryQos(discoveryQos)
                                                                      .withMessagingQos(new MessagingQos())
                                                                      .build();

            CountDownLatch countDownLatch = new CountDownLatch(1);
            proxy.add(1, 1, messageId -> {
                result.append(messageId);
                countDownLatch.countDown();
            });
            countDownLatch.await();
        } catch (Exception e) {
            logger.error(result.toString());
            result.append("SIT RESULT error: stateless async JEE consumer -> ")
                  .append(domain)
                  .append("\nException: ")
                  .append(e.toString());
            Util.addStacktraceToResultString(e, result);
        }
        result.append("\n");
    }

}
