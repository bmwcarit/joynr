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

import com.google.inject.Inject;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;

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

    private static final String[] gbids = new String[]{ "joynrdefaultgbid", "othergbid" };

    private ServiceLocator serviceLocator;

    DiscoveryQos discoveryQos;
    DiscoveryQos statelessAsyncDiscoveryQos;
    MessagingQos messagingQos;
    String[] configuredDomains;
    String[] domainsToBeSent;

    @Inject
    public SitControllerConsumerRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes
        discoveryQos.setRetryIntervalMs(5000); // 5 seconds

        statelessAsyncDiscoveryQos = new DiscoveryQos();
        statelessAsyncDiscoveryQos.setDiscoveryTimeoutMs(discoveryQos.getDiscoveryTimeoutMs());
        statelessAsyncDiscoveryQos.setRetryIntervalMs(discoveryQos.getRetryIntervalMs());
        statelessAsyncDiscoveryQos.setCacheMaxAgeMs(DiscoveryQos.NO_MAX_AGE); // use provisioned DiscoveryEntry
        statelessAsyncDiscoveryQos.setArbitrationStrategy(ArbitrationStrategy.FixedChannel);

        messagingQos = new MessagingQos(360000); // 6 Minutes

        String configuredDomainString = System.getenv("SIT_DOMAINS");
        if (configuredDomainString != null) {
            configuredDomains = configuredDomainString.split(",");
        } else {
            throw new JoynrIllegalStateException("No domains have been configured for the controller!");
        }
        for (int i = 0; i < configuredDomains.length; i++) {
            configuredDomains[i] = configuredDomains[i].trim();
        }

        String domainsToBeSentString = System.getenv("SIT_DOMAINS_TO_BE_SENT");
        if (domainsToBeSentString != null) {
            domainsToBeSent = domainsToBeSentString.split(",");
        } else {
            throw new JoynrIllegalStateException("No domains have been configured to be sent to the applications!");
        }
        for (int i = 0; i < domainsToBeSent.length; i++) {
            domainsToBeSent[i] = domainsToBeSent[i].trim();
        }

        if (domainsToBeSent.length != configuredDomains.length) {
            throw new JoynrIllegalStateException("Amount of testsubject domains and test execution domains has to be equal!");
        }
    }

    private void waitForJoynrEndpoint(String domain, DiscoveryQos discoveryQos) {
        waitForJoynrEndpoint(domain, discoveryQos, 1);
    }

    private void waitForJoynrEndpoint(String domain, DiscoveryQos discoveryQos, int tries) {
        SitControllerSync sitApp = serviceLocator.builder(SitControllerSync.class, domain)
                                                 .withDiscoveryQos(discoveryQos)
                                                 .withMessagingQos(messagingQos)
                                                 .withGbids(gbids)
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
        for (String configuredDomain : configuredDomains) {
            try {

                waitForJoynrEndpoint(configuredDomain + "_" + SIT_DOMAIN_PREFIX + ".jee", discoveryQos);

            } catch (Exception e) {
                String errorMsg = "SIT RESULT error: sit-jee-app with domain " + configuredDomain
                        + " failed to start in time: " + e;
                logger.error(errorMsg);
                return errorMsg;
            }
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

        for (int i = 0; i < configuredDomains.length; i++) {
            try {
                SitControllerSync sitControllerJeeApp = serviceLocator.builder(SitControllerSync.class,
                                                                               configuredDomains[i] + "_"
                                                                                       + SIT_DOMAIN_PREFIX + ".jee")
                                                                      .withDiscoveryQos(discoveryQos)
                                                                      .withMessagingQos(messagingQos)
                                                                      .withGbids(gbids)
                                                                      .build();
                result.append(new String(Base64.getMimeDecoder()
                                               .decode(sitControllerJeeApp.triggerTests(domainsToBeSent[i], false)
                                                                          .getBytes(StandardCharsets.ISO_8859_1)),
                                         StandardCharsets.UTF_8));
            } catch (Exception e) {
                String errorMsg = "SIT RESULT error: triggerTests of sit-jee-app with domain " + configuredDomains[i]
                        + " failed: " + e;
                logger.error(errorMsg);
                result.append("\n").append(errorMsg);
            }
        }

        try {
            SitControllerSync sitControllerJeeApp = serviceLocator.builder(SitControllerSync.class,
                                                                           configuredDomains[0] + "_"
                                                                                   + SIT_DOMAIN_PREFIX + ".jee")
                                                                  .withDiscoveryQos(discoveryQos)
                                                                  .withMessagingQos(messagingQos)
                                                                  .withGbids(gbids)
                                                                  .build();
            result.append(new String(Base64.getMimeDecoder()
                                           .decode(sitControllerJeeApp.triggerTests(domainsToBeSent[0], true)
                                                                      .getBytes(StandardCharsets.ISO_8859_1)),
                                     StandardCharsets.UTF_8));
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: triggerTests of sit-jee-app with domain " + configuredDomains[0]
                    + " failed: " + e;
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
                                                                                  .withGbids(gbids)
                                                                                  .build();

            statelessAsyncDiscoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD,
                                                          PARTICIPANT_ID_STATELESS_ASYNC_CONSUMER_NODE_2);
            SitControllerSync sitControllerJeeStatelessAsyncNode2 = serviceLocator.builder(SitControllerSync.class,
                                                                                           CONTROLLER_DOMAIN_PREFIX
                                                                                                   + ".jee-stateless-consumer")
                                                                                  .withDiscoveryQos(statelessAsyncDiscoveryQos)
                                                                                  .withMessagingQos(messagingQos)
                                                                                  .withGbids(gbids)
                                                                                  .build();

            result.append(sitControllerJeeStatelessAsyncNode1.triggerTests(configuredDomains[0], false));

            result.append(new String(Base64.getMimeDecoder()
                                           .decode(sitControllerJeeStatelessAsyncNode1.waitForStatelessResult(60000)
                                                                                      .getBytes(StandardCharsets.ISO_8859_1)),
                                     StandardCharsets.UTF_8));
            result.append("\n");
            result.append(new String(Base64.getMimeDecoder()
                                           .decode(sitControllerJeeStatelessAsyncNode2.waitForStatelessResult(60000)
                                                                                      .getBytes(StandardCharsets.ISO_8859_1)),
                                     StandardCharsets.UTF_8));
            result.append("\n");
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: triggerTests of sit-jee-stateless-consumer failed: " + e;
            logger.error(errorMsg, e);
            result.append("\n").append(errorMsg);
        }
        return result.toString();
    }

}
