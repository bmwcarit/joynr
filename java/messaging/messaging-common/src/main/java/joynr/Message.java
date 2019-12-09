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

    public enum MessageType {
        VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST("brq"), VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST(
                "mrq"), VALUE_MESSAGE_TYPE_MULTICAST("m"), VALUE_MESSAGE_TYPE_ONE_WAY(
                        "o"), VALUE_MESSAGE_TYPE_PUBLICATION("p"), VALUE_MESSAGE_TYPE_REPLY(
                                "rp"), VALUE_MESSAGE_TYPE_REQUEST("rq"), VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY(
                                        "srp"), VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST(
                                                "arq"), VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP("sst");

        private final String text;

        private MessageType(String s) {
            text = s;
        }

        @Override
        public String toString() {
            return this.text;
        }

        public static MessageType fromString(String text) {
            for (MessageType type : MessageType.values()) {
                if (type.text.equalsIgnoreCase(text)) {
                    return type;
                }
            }
            return null;
        }
    }

    public static final String CUSTOM_HEADER_PREFIX = "c-";
    public static final String HEADER_EFFORT = "ef";
    public static final String HEADER_ID = "id";
    public static final String HEADER_MSG_TYPE = "t";
    public static final String HEADER_REPLY_TO = "re";

    public static final String CUSTOM_HEADER_REQUEST_REPLY_ID = "z4";
    public static final String CUSTOM_HEADER_GBID_KEY = "gb";

    public static final String VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST = "brq";
    public static final String VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST = "mrq";
    public static final String VALUE_MESSAGE_TYPE_MULTICAST = "m";
    public static final String VALUE_MESSAGE_TYPE_ONE_WAY = "o";
    public static final String VALUE_MESSAGE_TYPE_PUBLICATION = "p";
    public static final String VALUE_MESSAGE_TYPE_REPLY = "rp";
    public static final String VALUE_MESSAGE_TYPE_REQUEST = "rq";
    public static final String VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY = "srp";
    public static final String VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "arq";
    public static final String VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP = "sst";

    private transient boolean localMessage;

    public boolean isLocalMessage() {
        return localMessage;
    }

    public void setLocalMessage(boolean message) {
        this.localMessage = message;
    }
}
