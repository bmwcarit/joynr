package io.joynr.messaging;

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

/**
 * Stores the base URL of the bounce proxy and composes URLs for channel
 * creation and message transmission.
 * 
 */

public class BounceProxyUrl {

    private static final String URL_PATH_SEPARATOR = "/";
    private final String bounceProxyBaseUrl;
    private String bounceProxyChannelsBaseUrl;
    private final static String CREATE_CHANNEL_QUERY_ITEM = "ccid";
    private final static String CHANNEL_PATH_SUFFIX = "channels";

    //    private final static String SEND_MESSAGE_PATH_APPENDIX = "message";

    public BounceProxyUrl(final String bounceProxyBaseUrl) {
        if (!bounceProxyBaseUrl.endsWith(URL_PATH_SEPARATOR)) {
            this.bounceProxyBaseUrl = bounceProxyBaseUrl + URL_PATH_SEPARATOR;
        } else {
            this.bounceProxyBaseUrl = bounceProxyBaseUrl;
        }

        this.bounceProxyChannelsBaseUrl = bounceProxyBaseUrl + CHANNEL_PATH_SUFFIX + URL_PATH_SEPARATOR;
    }

    public String buildTimeCheckUrl() {
        return bounceProxyBaseUrl + "time/";
    }

    public String buildCreateChannelUrl(final String mcid) {
        return bounceProxyChannelsBaseUrl + "?" + CREATE_CHANNEL_QUERY_ITEM + "=" + mcid;
    }

    public String getSendUrl(final String channelId) {
        StringBuilder sendUrl = new StringBuilder(bounceProxyChannelsBaseUrl);
        sendUrl.append(channelId);
        sendUrl.append(URL_PATH_SEPARATOR);
        //        sendUrl.append(SEND_MESSAGE_PATH_APPENDIX);
        //        sendUrl.append(URL_PATH_SEPARATOR);
        return sendUrl.toString();
    }

    public String getBounceProxyBaseUrl() {
        return bounceProxyBaseUrl;
    }
}
