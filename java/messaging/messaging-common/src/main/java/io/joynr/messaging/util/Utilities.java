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
package io.joynr.messaging.util;

import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;

import com.google.common.collect.Lists;

import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;

import joynr.ImmutableMessage;

public class Utilities {
    private static final Logger logger = LoggerFactory.getLogger(Utilities.class);

    /**
     * return an array of the params, useful for logging in slf4j
     *
     * @param args Arguments which should be returned as array.
     * @return array build from arguments
     */
    public static Object[] toArray(Object... args) {
        return args;
    }

    /**
     * This method splits the input string into several json objects. This is
     * needed because Atmosphere sends more than one json in a single response.
     * <br>
     * For example for the input {{test}{test2}}{test3} it would produce the
     * following list: [{{test}{test2}}, {test3}]
     * @param combinedJsonString The JSON input to be splitted
     * @return List of JSON strings
     */
    public static List<String> splitJson(String combinedJsonString) {
        List<String> result = Lists.newArrayList();

        int numberOfOpeningBraces = 0;
        boolean isInsideString = false;
        /*
         * A string starts with an unescaped " and ends with an unescaped "} or
         * { within a string must be ignored.
         */
        StringBuilder jsonBuffer = new StringBuilder();
        for (int i = 0; i < combinedJsonString.length(); i++) {
            char c = combinedJsonString.charAt(i);
            if (c == '"' && i > 0 && combinedJsonString.charAt(i - 1) != '\\') {
                // only switch insideString if " is not escaped
                isInsideString = !isInsideString;
            }
            if (c == '{' && !isInsideString) {
                numberOfOpeningBraces++;
            } else if (c == '}' && !isInsideString) {
                numberOfOpeningBraces--;
            }

            jsonBuffer.append(c);

            if (numberOfOpeningBraces == 0 && jsonBuffer.length() != 0) {
                // Prevent empty strings to be added to the result list in case
                // there are spaces between JSON objects
                if (jsonBuffer.toString().charAt(0) == '{') {
                    result.add(jsonBuffer.toString());
                }
                jsonBuffer.setLength(0);
            }
        }

        return result;
    }

    public static List<ImmutableMessage> splitSMRF(byte[] combinedSMRFMessages) throws EncodingException,
                                                                               UnsuppportedVersionException {
        List<ImmutableMessage> result = Lists.newArrayList();
        byte[] remainingData = combinedSMRFMessages;

        while (remainingData.length > 0) {
            ImmutableMessage currentMessage = new ImmutableMessage(remainingData);
            int currentMessageSize = currentMessage.getMessageSize();

            // A message size of 0 may lead to an infinite loop
            if (currentMessageSize <= 0) {
                logger.error("One message in a SMRF message sequence had a size <= 0. Dropping remaining messages.");
                break;
            }

            remainingData = Arrays.copyOfRange(remainingData, currentMessageSize, remainingData.length);

            result.add(currentMessage);
        }

        return result;
    }

    /**
     * Returns whether the session ID is encoded into the URL.
     *
     * @param encodedUrl
     *            the url to check
     * @param sessionIdName
     *            the name of the session ID, e.g. jsessionid
     * @return boolean value, true if session ID is encoded into the URL.
     */
    public static boolean isSessionEncodedInUrl(String encodedUrl, String sessionIdName) {
        int sessionIdIndex = encodedUrl.indexOf(getSessionIdSubstring(sessionIdName));
        return sessionIdIndex >= 0;
    }

    /**
     * Returns the URL without the session encoded into the URL.
     *
     * @param url
     *            the url to de-code
     * @param sessionIdName
     *            the name of the session ID, e.g. jsessionid
     * @return url without session id information or url, if no session was
     *         encoded into the URL
     */
    public static String getUrlWithoutSessionId(String url, String sessionIdName) {

        if (isSessionEncodedInUrl(url, sessionIdName)) {
            return url.substring(0, url.indexOf(getSessionIdSubstring(sessionIdName)));
        }
        return url;
    }

    /**
     * Returns the session id from a URL. It is expected that the URL contains a
     * session, which can be checked by calling
     * {@link Utilities#isSessionEncodedInUrl(String, String)}
     *
     * @param url
     *            the url to get the session ID from
     * @param sessionIdName
     *            the name of the session ID, e.g. jsessionid
     * @return
     *            the session ID
     */
    public static String getSessionId(String url, String sessionIdName) {
        String sessionIdSubstring = getSessionIdSubstring(sessionIdName);
        String sessionId = url.substring(url.indexOf(sessionIdSubstring) + sessionIdSubstring.length());

        if (sessionId.endsWith("/")) {
            sessionId = sessionId.substring(0, sessionId.length() - 1);
        }

        return sessionId;
    }

    private static String getSessionIdSubstring(String sessionIdName) {
        return ";" + sessionIdName + "=";
    }

    /**
     * Returns a url with the session encoded into the URL
     *
     * @param url the URL to encode
     * @param sessionIdName the name of the session ID, e.g. jsessionid
     * @param sessionId the session ID
     * @return URL with the session encoded into the URL
     */
    public static String getSessionEncodedUrl(String url, String sessionIdName, String sessionId) {
        return url + getSessionIdSubstring(sessionIdName) + sessionId;
    }
}
