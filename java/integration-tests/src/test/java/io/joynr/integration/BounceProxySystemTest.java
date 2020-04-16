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
package io.joynr.integration;

import static io.joynr.util.JoynrUtil.createUuidString;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.LinkedList;
import java.util.List;

import org.apache.http.HttpEntity;
import org.apache.http.HttpStatus;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.ContentType;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.util.Utilities;

/**
 * Bounce proxy test for an already running bounce proxy server, that prints out
 * statistics on how many messages are received or lost. This especially makes
 * sense for bounce proxies in a clustered environment that the tester can't
 * control, e.g. with load balancers in front. In this case it could happen that
 * messages are lost because they are routed to a different bounce proxy
 * instance than expected. <br>
 * This test is meant to be executed manually on the command line. The tester
 * should check manually if the number of received messages meet his
 * expectations. As the bounce proxy server isn't started in a controlled test
 * environment, it doesn't make sense to test for certain conditions.<br>
 * The test sets up a channel and sends a configurable number of messages to the
 * server. Some statistics about how many messages were sent and received are
 * printed on the command line.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxySystemTest {

    private static final Logger logger = LoggerFactory.getLogger(BounceProxySystemTest.class);

    private final String bounceProxyUrl;
    private final int noOfMessages;

    private final CloseableHttpClient httpclient;

    public BounceProxySystemTest(String bounceProxyUrl, int noOfMessages) {
        this.bounceProxyUrl = bounceProxyUrl;
        this.noOfMessages = noOfMessages;
        this.httpclient = HttpClients.createDefault();
    }

    private void run() throws Exception {

        logger.info("================================");
        logger.info("=== Bounce Proxy System Test ===");
        logger.info("================================");

        String channelId = "channel-" + createUuidString();
        String trackingId = "trackingId-" + createUuidString();

        logger.info("1) Channel Creation");
        String channelUrl = createChannel(channelId, trackingId);
        logger.info("Channel location: {}", channelUrl);

        logger.info("2) Message Sending");

        int rejected = 0;

        for (int i = 0; i < noOfMessages; i++) {
            if (!postMessage(channelUrl, "msg" + i)) {
                rejected++;
            }
            Thread.sleep(100);
        }
        logger.info("Sent {} messages:", noOfMessages);
        logger.info("   {} rejected messages, {} messages waiting for retrieval", rejected, noOfMessages - rejected);

        logger.info("3) Message Retrieving");

        List<String> messages = retrieveMessages(channelUrl, trackingId);
        logger.info("Received {} messages", messages.size());

        logger.info("4) Channel Deletion");
        deleteChannel(channelUrl);

        logger.info("Bye bye!");
    }

    private String createChannel(String channelId, String trackingId) throws Exception {

        final String url = bounceProxyUrl + "channels/?ccid=" + channelId;
        logger.info("Creating channel at {}..." + url);

        HttpPost postCreateChannel = new HttpPost(url.trim());
        postCreateChannel.addHeader("X-Atmosphere-tracking-id", trackingId);

        CloseableHttpResponse response = null;
        try {
            response = httpclient.execute(postCreateChannel);
            logger.info("Returned {}", response.getStatusLine());

            int statusCode = response.getStatusLine().getStatusCode();
            switch (statusCode) {
            case HttpStatus.SC_CREATED:
            case HttpStatus.SC_OK:
                return response.getFirstHeader("Location").getValue();

            default:
                logger.error("Failed to create channel {}", channelId);
                printBody(response.getEntity());
                throw new RuntimeException("Could not proceed with messaging as channel could not be created");
            }
        } finally {
            if (response != null)
                response.close();
        }

    }

    private boolean postMessage(String channelUrl, String msgId) throws Exception {

        String messageUrl = encodeSendUrl(channelUrl);

        logger.info("Sending message {} to {} ... ", msgId, messageUrl);
        String serializedMessage = createJoynrMessage(msgId);

        HttpPost postMessage = new HttpPost(messageUrl);
        postMessage.addHeader("Content-Type", ContentType.APPLICATION_JSON.toString());

        postMessage.setEntity(new StringEntity(serializedMessage));

        CloseableHttpResponse response = httpclient.execute(postMessage);
        int statusCode = response.getStatusLine().getStatusCode();
        logger.info("Returned {}", response.getStatusLine());
        response.close();

        return statusCode == HttpStatus.SC_CREATED;
    }

    private List<String> retrieveMessages(String channelUrl, String trackingId) throws Exception {

        logger.info("Polling for messages... ");

        HttpGet getMessages = new HttpGet(channelUrl);
        getMessages.setHeader("X-Atmosphere-tracking-id", trackingId);

        CloseableHttpResponse response = null;

        try {

            response = httpclient.execute(getMessages);
            logger.info("Returned {}", response.getStatusLine());

            if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                return null;
            }

            HttpEntity entity = response.getEntity();

            BufferedReader reader = new BufferedReader(new InputStreamReader(entity.getContent(), "UTF-8"));
            String body = reader.readLine();
            reader.close();

            if (body == null) {
                return new LinkedList<String>();
            }

            return Utilities.splitJson(body);
        } finally {
            if (response != null)
                response.close();
        }
    }

    private void deleteChannel(String channelUrl) throws Exception {

        logger.info("Delete channel {} ...", channelUrl);

        HttpDelete httpDelete = new HttpDelete(channelUrl);
        CloseableHttpResponse response = null;

        try {
            response = httpclient.execute(httpDelete);
            logger.info("Returned {}", response.getStatusLine());
        } finally {
            if (response != null)
                response.close();
        }
    }

    private String createJoynrMessage(String msgId) {

        return "{ \"_typeName\": \"joynr.JoynrMessage\", \"type\": \"request\", \"header\": { \"expiryDate\": \""
                + System.currentTimeMillis() + 100000l + "\", \"msgId\": \"" + msgId
                + "\" }, \"payload\": \"payload199-" + createUuidString() + "\" }";
    }

    private void printBody(HttpEntity entity) {

        BufferedReader reader = null;

        try {
            String charsetEncoding = null;
            if (entity.getContentEncoding() != null) {
                charsetEncoding = entity.getContentEncoding().getValue();
            } else {
                charsetEncoding = "UTF-8";
            }

            reader = new BufferedReader(new InputStreamReader(entity.getContent(), charsetEncoding));
            String body = reader.readLine();

            if (isHtml(entity)) {
                String fileLocation = writeToFile(body, charsetEncoding);
                logger.info("HTML output saved in file at {}", fileLocation);
            } else {
                logger.info("Body: {}", body);
            }
        } catch (IOException e) {
            logger.warn("Error printing body: {}", e.getMessage());
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    logger.warn("Error reading body: {}", e.getMessage());
                }
            }
        }
    }

    private String writeToFile(String body, String charsetEncoding) {
        File file;
        BufferedWriter writer;
        try {
            file = File.createTempFile("JoynBackendTest", ".html");

            writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), charsetEncoding));
            writer.write(body);
            writer.close();
            return file.getAbsolutePath();
        } catch (IOException e) {
            logger.warn("Error writing to file: body: {}, error: {}", body, e.getMessage());
            return "not available";
        }
    }

    private boolean isHtml(HttpEntity entity) {
        return ContentType.TEXT_HTML.toString()
                                    .toLowerCase()
                                    .startsWith(entity.getContentType().getValue().toLowerCase());
    }

    private String encodeSendUrl(String encodedChannelUrl) {

        String sessionIdSubstring = ";jsessionid=";

        boolean isEncoded = false;
        int sessionIdIndex = encodedChannelUrl.indexOf(sessionIdSubstring);

        isEncoded = sessionIdIndex >= 0;

        if (isEncoded) {
            String channelUrlWithoutSessionId = encodedChannelUrl.substring(0, sessionIdIndex);
            String sessionId = encodedChannelUrl.substring(sessionIdIndex + sessionIdSubstring.length());

            if (sessionId.endsWith("/")) {
                sessionId = sessionId.substring(0, sessionId.length() - 1);
            }

            return channelUrlWithoutSessionId + "message/" + sessionIdSubstring + sessionId;

        } else {
            return encodedChannelUrl + "message/";
        }
    }

    public static void main(String[] args) {

        if (args.length != 2) {
            logger.error("Expecting arguments <bounceProxyUrl> <msgCount>");
            logger.error("   bounceProxyUrl: the URL to send the initial createChannel request to");
            logger.error("   msgCount: the number of messages to send to the bounce proxy");
            return;
        }

        String bpUrl = args[0];
        int msgCount = Integer.parseInt(args[1]);

        BounceProxySystemTest test = new BounceProxySystemTest(bpUrl, msgCount);
        try {
            test.run();
        } catch (Exception e) {
            logger.error("Error running tests: ", e);
        }

    }

}
