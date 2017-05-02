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
package joynr;

public abstract class Message {
    public static final String CUSTOM_HEADER_PREFIX = "custom-";
    public static final String HEADER_EFFORT = "effort";
    public static final String HEADER_ID = "id";
    public static final String HEADER_MSG_TYPE = "type";
    public static final String HEADER_REPLY_TO = "replyTo";

    public static final String CUSTOM_HEADER_REQUEST_REPLY_ID = "z4";
    public static final String VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST = "broadcastSubscriptionRequest";
    public static final String VALUE_MESSAGE_TYPE_MULTICAST = "multicast";
    public static final String VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST = "multicastSubscriptionRequest";
    public static final String VALUE_MESSAGE_TYPE_ONE_WAY = "oneWay";
    public static final String VALUE_MESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
    public static final String VALUE_MESSAGE_TYPE_REPLY = "reply";
    public static final String VALUE_MESSAGE_TYPE_REQUEST = "request";
    public static final String VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
    public static final String VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
    public static final String VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";

    private transient boolean localMessage;

    public boolean isLocalMessage() {
        return localMessage;
    }

    public void setLocalMessage(boolean message) {
        this.localMessage = message;
    }
}
