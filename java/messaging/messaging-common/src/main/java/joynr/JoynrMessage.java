package joynr;

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

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import com.fasterxml.jackson.annotation.JsonIgnore;
import io.joynr.common.ExpiryDate;
import io.joynr.subtypes.JoynrType;

import java.io.Serializable;

/**
 * Storage class to keep the type, header and payload of a message.
 */
public class JoynrMessage implements JoynrType {
    private static final long serialVersionUID = 1L;
    public static final String HEADER_NAME_REPLY_CHANNELID = "replyChannelId";
    public static final String HEADER_NAME_CONTENT_TYPE = "contentType";
    public static final String HEADER_NAME_EXPIRY_DATE = "expiryDate";
    public static final String HEADER_NAME_EFFORT = "effort";
    public static final String HEADER_NAME_MESSAGE_ID = "msgId";
    public static final String HEADER_NAME_CREATOR_USER_ID = "creator";

    public static final String HEADER_NAME_TO_PARTICIPANT_ID = "to";
    public static final String HEADER_NAME_FROM_PARTICIPANT_ID = "from";

    public static final String MESSAGE_TYPE_ONE_WAY = "oneWay";
    public static final String MESSAGE_TYPE_REQUEST = "request";
    public static final String MESSAGE_TYPE_REPLY = "reply";
    public static final String MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
    public static final String MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST = "broadcastSubscriptionRequest";
    public static final String MESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
    public static final String MESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";
    public static final String MESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
    public static final String MESSAGE_TYPE_MULTICAST = "multicast";

    public static final String MESSAGE_CUSTOM_HEADER_PREFIX = "custom-";
    public static final String CONTENT_TYPE_TEXT_PLAIN = "text/plain";
    public static final String CONTENT_TYPE_APPLICATION_JSON = "application/json";
    public static final String MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST = "multicastSubscriptionRequest";

    private String type;
    private Map<String, String> header;
    private String payload;
    private transient Boolean receivedFromGlobal;
    private transient Boolean localMessage = false;
    private transient HashMap<String, Serializable> context;

    public JoynrMessage() {
        this(null, new HashMap<String, String>(), null);
    }

    public JoynrMessage(String type, Map<String, String> header, String payload) {
        this.type = type;
        this.header = header;
        this.payload = payload;

        if (!this.header.containsKey(HEADER_NAME_MESSAGE_ID)) {
            String msgId = UUID.randomUUID().toString();
            this.header.put(HEADER_NAME_MESSAGE_ID, msgId);
        }
    }

    public JoynrMessage(JoynrMessage message) {
        this.type = message.type;
        this.header = new HashMap<String, String>(message.getHeader());
        this.payload = message.payload;
        this.receivedFromGlobal = message.receivedFromGlobal;
        this.localMessage = message.localMessage;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getHeaderValue(String key) {
        return header.get(key);
    }

    public void setHeaderValue(String key, String value) {
        header.put(key, value);
    }

    public Map<String, String> getHeader() {
        return header;
    }

    public void setCustomHeaders(Map<String, String> customHeaders) {
        for (Map.Entry<String, String> entry : customHeaders.entrySet()) {
            header.put(MESSAGE_CUSTOM_HEADER_PREFIX + entry.getKey(), entry.getValue());
        }
    }

    @JsonIgnore
    public Map<String, String> getCustomHeaders() {
        Map<String, String> customHeaders = new HashMap<>();
        for (Map.Entry<String, String> entry : header.entrySet()) {
            if (entry.getKey().startsWith(MESSAGE_CUSTOM_HEADER_PREFIX)) {
                String key = entry.getKey().replaceFirst("^" + MESSAGE_CUSTOM_HEADER_PREFIX, "");
                customHeaders.put(key, entry.getValue());
            }
        }
        return customHeaders;
    }

    /**
     * Adds header entries to the already existing ones. If a header entry was already set, its value is replaced with
     * the new one.
     *
     * @param newHeaders
     *            the header entries to add
     */
    public void setHeader(Map<String, String> newHeaders) {
        this.header.putAll(newHeaders);
    }

    public String getPayload() {
        return payload;
    }

    public void setPayload(String payload) {
        this.payload = payload;
    }

    @JsonIgnore
    public boolean isReceivedFromGlobal() {
        return Boolean.TRUE.equals(receivedFromGlobal);
    }

    public void setReceivedFromGlobal(boolean receivedFromGlobal) {
        this.receivedFromGlobal = receivedFromGlobal;
    }

    /**
     * Gets localMessage attribute
     *
     * Transient flag isLocalMessage is used to mark messages to be sent
     * to a provider that is registered on the local cluster controller.
     *
     * @return True, if the message is to
     * be sent to a provider that is registered on the
     * local cluster controller, false otherwise.
     */
    @JsonIgnore
    public boolean isLocalMessage() {
        return Boolean.TRUE.equals(localMessage);
    }

    /**
     * Sets localMessage attribute
     *
     * @param localMessage True, if the message is to
     * be sent to a provider that is registered on the
     * local cluster controller, false otherwise.
     */
    public void setLocalMessage(boolean localMessage) {
        this.localMessage = localMessage;
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("message: ");
        stringBuilder.append(getId());
        stringBuilder.append("\r\n  type=");
        stringBuilder.append(type);
        stringBuilder.append("\r\n  from=");
        stringBuilder.append(header.get(HEADER_NAME_FROM_PARTICIPANT_ID));
        stringBuilder.append("\r\n  to=");
        stringBuilder.append(header.get(HEADER_NAME_TO_PARTICIPANT_ID));
        stringBuilder.append("\r\n  header=");
        stringBuilder.append(header);
        stringBuilder.append("\r\n  payload=");
        if (payload == null) {
            stringBuilder.append("null");
        } else if (payload.length() < 1000) {
            stringBuilder.append(payload);
        } else {
            stringBuilder.append(payload.substring(0, 999) + "...");
        }
        stringBuilder.append("\r\n");
        return stringBuilder.toString();
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((header == null) ? 0 : header.hashCode());
        result = prime * result + ((payload == null) ? 0 : payload.hashCode());
        result = prime * result + ((type == null) ? 0 : type.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        JoynrMessage other = (JoynrMessage) obj;
        if (header == null) {
            if (other.header != null) {
                return false;
            }
        } else if (!header.equals(other.header)) {
            return false;
        }
        if (payload == null) {
            if (other.payload != null) {
                return false;
            }
        } else if (!payload.equals(other.payload)) {
            return false;
        }
        if (type == null) {
            if (other.type != null) {
                return false;
            }
        } else if (!type.equals(other.type)) {
            return false;
        }
        return true;
    }

    public String toLogMessage() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("type=");
        stringBuilder.append(type);
        stringBuilder.append(", header=");
        stringBuilder.append(header);
        return stringBuilder.toString();
    }

    @JsonIgnore
    public String getId() {
        return getHeaderValue(JoynrMessage.HEADER_NAME_MESSAGE_ID);
    }

    @JsonIgnore
    public String getCreatorUserId() {
        return getHeaderValue(HEADER_NAME_CREATOR_USER_ID);
    }

    @JsonIgnore
    public void setCreatorUserId(String creatorUserId) {
        setHeaderValue(HEADER_NAME_CREATOR_USER_ID, creatorUserId);
    }

    @JsonIgnore
    /**
     *
     * @return sender's ParticipantId
     */
    public String getFrom() {
        return getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID);
    }

    @JsonIgnore
    /**
     *
     * @param fromParticipantId sets the sender's ParticipantId
     */
    public void setFrom(String fromParticipantId) {
        setHeaderValue(HEADER_NAME_FROM_PARTICIPANT_ID, fromParticipantId);
    }

    @JsonIgnore
    /**
     *
     * @return receiver's ParticipantId
     */
    public String getTo() {
        return getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
    }

    @JsonIgnore
    /**
     * Sets the receiver's
     * @param toParticipantId
     */
    public void setTo(String toParticipantId) {
        setHeaderValue(HEADER_NAME_TO_PARTICIPANT_ID, toParticipantId);
    }

    @JsonIgnore
    /**
     *
     * @return Absolute time in ms when the message will expire and be discarded
     */
    public long getExpiryDate() {
        try {
            return Long.parseLong(getHeaderValue(JoynrMessage.HEADER_NAME_EXPIRY_DATE));
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    @JsonIgnore
    /**
     *
     * @param expirationDate
     *            the date/time when the message will expire and be discarded
     */
    public void setExpirationDate(ExpiryDate expirationDate) {
        setHeaderValue(HEADER_NAME_EXPIRY_DATE, Long.toString(expirationDate.getValue()));

    }

    @JsonIgnore
    /**
     *
     * @return the channelId of the cluster controller that is sending the message
     */
    public String getReplyTo() {
        return getHeaderValue(HEADER_NAME_REPLY_CHANNELID);
    }

    @JsonIgnore
    /**
     *
     * @param replyToChannelId
     *            the channelId of the cluster controller that is sending the message
     */
    public void setReplyTo(String replyToChannelId) {
        setHeaderValue(HEADER_NAME_REPLY_CHANNELID, replyToChannelId);
    }

    public void setContext(HashMap<String, Serializable> context) {
        this.context = context;
    }

    public Map<String, Serializable> getContext() {
        return context;
    }
}
