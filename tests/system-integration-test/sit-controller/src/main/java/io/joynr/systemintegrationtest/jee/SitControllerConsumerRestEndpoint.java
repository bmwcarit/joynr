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
package io.joynr.systemintegrationtest.jee;

import java.nio.charset.StandardCharsets;
import java.util.Base64;

import javax.inject.Inject;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.ArbitrationConstants;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.messaging.MessagingQos;
import joynr.test.SitControllerSync;

/**
 * Exposes a REST endpoint with which the consumer side joynr system integration tests controller can be triggered.
 */
@Path("/")
@Produces(MediaType.APPLICATION_JSON)
public class SitControllerConsumerRestEndpoint {

    private static final Logger logger = LoggerFactory.getLogger(SitControllerConsumerRestEndpoint.class);

    private static final String SIT_DOMAIN_PREFIX = "io.joynr.systemintegrationtest";
    private static final String CONTROLLER_DOMAIN_PREFIX = SIT_DOMAIN_PREFIX + ".controller";

    private static final String PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_1 = "sit-controller.stateless-cons-1";
    private static final String PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_2 = "sit-controller.stateless-cons-2";

    private ServiceLocator serviceLocator;

    DiscoveryQos discoveryQos;
    DiscoveryQos statelessAsyncDiscoveryQos;
    MessagingQos messagingQos;

    @Inject
    public SitControllerConsumerRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes
        discoveryQos.setRetryIntervalMs(5000); // 2 seconds

        statelessAsyncDiscoveryQos = new DiscoveryQos();
        statelessAsyncDiscoveryQos.setDiscoveryTimeoutMs(discoveryQos.getDiscoveryTimeoutMs());
        statelessAsyncDiscoveryQos.setRetryIntervalMs(discoveryQos.getRetryIntervalMs());
        statelessAsyncDiscoveryQos.setCacheMaxAgeMs(DiscoveryQos.NO_MAX_AGE); // use provisioned DiscoveryEntry
        statelessAsyncDiscoveryQos.setArbitrationStrategy(ArbitrationStrategy.FixedChannel);

        messagingQos = new MessagingQos(30000); // 30 seconds
    }

    private void waitForJoynrEndpoint(String domain, DiscoveryQos discoveryQos) {
        waitForJoynrEndpoint(domain, discoveryQos, 1);
    }

    private void waitForJoynrEndpoint(String domain, DiscoveryQos discoveryQos, int tries) {
        SitControllerSync sitApp = serviceLocator.builder(SitControllerSync.class, domain)
                                                 .withDiscoveryQos(discoveryQos)
                                                 .withMessagingQos(messagingQos)
                                                 .build();
        String result = "";
        while (tries > 0) {
            tries--;
            try {
                result = sitApp.ping();
            } catch (Exception e) {
                if (tries <= 0) {
                    throw e;
                }
                logger.debug("Retry ping for domain " + domain + " after error: " + e);
            }

        }
        logger.info("waitForJoynrEndpoint " + domain + " returned: " + result);
        if (!"OK".equals(result)) {
            throw new JoynrIllegalStateException("Ping returned unexpected result: \"" + result + "\"");
        }
    }

    @GET
    @Path("/ping")
    public String ping() {
        logger.info("ping called");
        try {
            waitForJoynrEndpoint(CONTROLLER_DOMAIN_PREFIX + ".jee-app", discoveryQos);
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: sit-jee-app failed to start in time: " + e;
            logger.error(errorMsg);
            return errorMsg;
        }
        try {
            // Retry ping 3 times because DiscoveryEntries of the clustered providers are provisioned
            // (the providers might not yet be ready to handle the request)
            statelessAsyncDiscoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD,
                                                          PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_1);
            waitForJoynrEndpoint(CONTROLLER_DOMAIN_PREFIX + ".jee-stateless-consumer", statelessAsyncDiscoveryQos, 3);
            statelessAsyncDiscoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD,
                                                          PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_2);
            waitForJoynrEndpoint(CONTROLLER_DOMAIN_PREFIX + ".jee-stateless-consumer", statelessAsyncDiscoveryQos, 3);
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: sit-jee-stateless-async-consumer failed to start in time: " + e;
            logger.error(errorMsg);
            return errorMsg;
        }
        return "OK";
    }

    @GET
    @Path("/test")
    public String triggerTests() {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();

        try {
            SitControllerSync sitControllerJeeApp = serviceLocator.builder(SitControllerSync.class,
                                                                           CONTROLLER_DOMAIN_PREFIX + ".jee-app")
                                                                  .withDiscoveryQos(discoveryQos)
                                                                  .withMessagingQos(messagingQos)
                                                                  .build();
            result.append(new String(Base64.getDecoder()
                                           .decode(sitControllerJeeApp.triggerTests()
                                                                      .getBytes(StandardCharsets.ISO_8859_1)),
                                     StandardCharsets.UTF_8));
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: triggerTests of sit-jee-app failed: " + e;
            logger.error(errorMsg);
            result.append("\n").append(errorMsg);
        }

        try {
            statelessAsyncDiscoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD,
                                                          PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_1);
            SitControllerSync sitControllerJeeStatelessAsyncNode1 = serviceLocator.builder(SitControllerSync.class,
                                                                                           CONTROLLER_DOMAIN_PREFIX
                                                                                                   + ".jee-stateless-consumer")
                                                                                  .withDiscoveryQos(statelessAsyncDiscoveryQos)
                                                                                  .withMessagingQos(messagingQos)
                                                                                  .build();

            statelessAsyncDiscoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD,
                                                          PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_2);
            SitControllerSync sitControllerJeeStatelessAsyncNode2 = serviceLocator.builder(SitControllerSync.class,
                                                                                           CONTROLLER_DOMAIN_PREFIX
                                                                                                   + ".jee-stateless-consumer")
                                                                                  .withDiscoveryQos(statelessAsyncDiscoveryQos)
                                                                                  .withMessagingQos(messagingQos)
                                                                                  .build();

            result.append(sitControllerJeeStatelessAsyncNode1.triggerTests());

            result.append(new String(Base64.getDecoder()
                                           .decode(sitControllerJeeStatelessAsyncNode1.waitForStatelessResult(60000)
                                                                                      .getBytes(StandardCharsets.ISO_8859_1)),
                                     StandardCharsets.UTF_8));
            result.append("\n");
            result.append(new String(Base64.getDecoder()
                                           .decode(sitControllerJeeStatelessAsyncNode2.waitForStatelessResult(60000)
                                                                                      .getBytes(StandardCharsets.ISO_8859_1)),
                                     StandardCharsets.UTF_8));
            result.append("\n");
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: triggerTests of sit-jee-stateless-consumer failed: " + e;
            logger.error(errorMsg);
            result.append("\n").append(errorMsg);
        }
        return result.toString();
    }

}
