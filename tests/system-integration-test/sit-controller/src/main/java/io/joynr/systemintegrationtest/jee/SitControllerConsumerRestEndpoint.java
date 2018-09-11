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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

import javax.inject.Inject;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

    private ServiceLocator serviceLocator;

    @Inject
    public SitControllerConsumerRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
    }

    @GET
    @Path("/ping")
    public String ping() {
        logger.info("ping called");
        return "OK";
    }

    private void getResultViaHttp(String url, StringBuffer result, String resultPrefix) throws IOException {
        logger.info("getResultViaHttp called: " + url);
        HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
        conn.setRequestMethod("GET");
        conn.connect();
        int responseCode = conn.getResponseCode();
        if (responseCode != HttpURLConnection.HTTP_OK) {
            conn.disconnect();
            throw new JoynrIllegalStateException("Unexpected response code from http request: " + url + ": "
                    + responseCode);
        }

        BufferedReader response = new BufferedReader(new InputStreamReader(conn.getInputStream()));
        int responseLengthBefore = result.length();
        response.lines().forEach(line -> {
            result.append("\n").append(resultPrefix);
            if (line != null && line.contains("SIT RESULT")) {
                result.append(line);
            } else {
                result.append("SIT RESULT error: result line does not contain \"SIT RESULT\": " + line);
            }
        });

        response.close();
        conn.disconnect();

        if (responseLengthBefore == result.length()) {
            throw new JoynrIllegalStateException("Response from http request is empty: : " + url);
        }
    }

    @GET
    @Path("/test")
    public String triggerTests() {
        logger.info("triggerTests called");
        StringBuffer result = new StringBuffer();

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes
        MessagingQos messagingQos = new MessagingQos();

        try {
            SitControllerSync sitControllerJeeStatelessAsync = serviceLocator.builder(SitControllerSync.class,
                                                                                      CONTROLLER_DOMAIN_PREFIX
                                                                                              + ".jee-stateless-consumer")
                                                                             .withDicoveryQos(discoveryQos)
                                                                             .withMessagingQos(messagingQos)
                                                                             .build();
            result.append(sitControllerJeeStatelessAsync.triggerTests());

            String urlNode1 = "http://sit-jee-stateless-consumer-node-1:8080/sit-jee-stateless-consumer/sit-controller/result";
            String resultPrefix = "sit-jee-stateless-consumer-node-1: ";
            getResultViaHttp(urlNode1, result, resultPrefix);

            String urlNode2 = "http://sit-jee-stateless-consumer-node-2:8080/sit-jee-stateless-consumer/sit-controller/result";
            resultPrefix = "sit-jee-stateless-consumer-node-2: ";
            getResultViaHttp(urlNode2, result, resultPrefix);
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: triggerTests of sit-jee-stateless-consumer failed: " + e;
            logger.error(errorMsg);
            result.append("\n").append(errorMsg);
        }
        return result.toString();
    }

}
