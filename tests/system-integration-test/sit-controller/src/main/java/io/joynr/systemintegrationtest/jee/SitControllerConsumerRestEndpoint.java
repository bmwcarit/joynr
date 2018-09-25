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
import java.util.Base64;

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

    private static final String URL_JEE_STATELESS_ASYNC_CONSUMER_NODE_1 = "http://sit-jee-stateless-consumer-node-1:8080/sit-jee-stateless-consumer/sit-controller/";
    private static final String URL_JEE_STATELESS_ASYNC_CONSUMER_NODE_2 = "http://sit-jee-stateless-consumer-node-2:8080/sit-jee-stateless-consumer/sit-controller/";
    private static final String PATH_PING = "ping";
    private static final int MAX_HTTP_CONNECT_ATTEMPTS = 30;

    private ServiceLocator serviceLocator;

    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

    @Inject
    public SitControllerConsumerRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(120000); // 2 Minutes
        discoveryQos.setRetryIntervalMs(5000); // 2 seconds
        messagingQos = new MessagingQos(60000); // 60 seconds
    }

    private void waitForJoynrEndpoint(String domain) {
        SitControllerSync sitApp = serviceLocator.builder(SitControllerSync.class, domain)
                                                 .withDicoveryQos(discoveryQos)
                                                 .withMessagingQos(messagingQos)
                                                 .build();
        String result = sitApp.ping();
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
            waitForJoynrEndpoint(CONTROLLER_DOMAIN_PREFIX + ".jee-app");
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: sit-jee-app failed to start in time: " + e;
            logger.error(errorMsg);
            return errorMsg;
        }
        try {
            waitForRestEndpoint("sit-jee-stateless-consumer-node-1",
                                URL_JEE_STATELESS_ASYNC_CONSUMER_NODE_1 + PATH_PING);
            waitForRestEndpoint("sit-jee-stateless-consumer-node-2",
                                URL_JEE_STATELESS_ASYNC_CONSUMER_NODE_2 + PATH_PING);
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: sit-jee-stateless-async-consumer failed to start in time: " + e;
            logger.error(errorMsg);
            return errorMsg;
        }
        return "OK";
    }

    private void waitForRestEndpoint(String serviceName, String url) throws IOException, InterruptedException {
        logger.info("waitForRestEndpoint called: " + url);
        HttpURLConnection conn = null;
        for (int i = 0; i < MAX_HTTP_CONNECT_ATTEMPTS; i++) {
            conn = connectViaHttp(url);
            int responseCode = conn.getResponseCode();
            if (responseCode == HttpURLConnection.HTTP_OK) {
                break;
            } else if (i == MAX_HTTP_CONNECT_ATTEMPTS - 1) {
                conn.disconnect();
                throw new JoynrIllegalStateException("Unexpected response code from http request: " + url + ": "
                        + responseCode);
            }
            conn.disconnect();
            logger.info(serviceName + ": ping not started yet ... (response code: " + responseCode + ")");
            Thread.sleep(2000);
        }

        BufferedReader response = new BufferedReader(new InputStreamReader(conn.getInputStream()));
        int responseLength = 0;
        String line;
        while ((line = response.readLine()) != null) {
            responseLength += line.length();
            logger.info("waitForRestEndpoint " + url + " returned: " + line);
        }

        response.close();
        conn.disconnect();

        if (responseLength <= 0) {
            throw new JoynrIllegalStateException("Unable to read response from http request: " + url
                    + ": reponse length: " + responseLength);
        }
        logger.info("waitForRestEndpoint done: " + url);
    }

    private HttpURLConnection connectViaHttp(String url) throws IOException {
        HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
        conn.setRequestMethod("GET");
        conn.connect();
        return conn;
    }

    private void getResultViaHttp(String url, StringBuffer result, String resultPrefix) throws IOException {
        logger.info("getResultViaHttp called: " + url);
        HttpURLConnection conn = connectViaHttp(url);

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

        try {
            SitControllerSync sitControllerJeeApp = serviceLocator.builder(SitControllerSync.class,
                                                                           CONTROLLER_DOMAIN_PREFIX + ".jee-app")
                                                                  .withDicoveryQos(discoveryQos)
                                                                  .withMessagingQos(messagingQos)
                                                                  .build();
            result.append(new String(Base64.getDecoder().decode(sitControllerJeeApp.triggerTests().getBytes())));
        } catch (Exception e) {
            String errorMsg = "SIT RESULT error: triggerTests of sit-jee-app failed: " + e;
            logger.error(errorMsg);
            result.append("\n").append(errorMsg);
        }

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
