/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import java.io.IOException;
import java.io.Serializable;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.databind.ser.std.StdSerializer;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.MessageDeserializer;
import io.joynr.smrf.MessageDeserializerImpl;
import io.joynr.smrf.UnsuppportedVersionException;
import io.joynr.util.ObjectMapper;

/**
 * This class wraps binary SMRF messages and provides access to the serialized attributes
 * of the message. This type is mainly used by the messaging layers and the message router.
 * {@link MutableMessage} objects will be converted to ImmutableMessage objects before they
 * are handed over to a sub-class of {@link io.joynr.messaging.routing.MessageRouter}.
 * It's not possible to modify a immutable message any more because the content may be signed.
 */
public class ImmutableMessage extends Message {

    private static final Logger logger = LoggerFactory.getLogger(ImmutableMessage.class);
    private transient boolean receivedFromGlobal;
    private transient boolean isMessageProcessed = false;
    private final MessageDeserializer messageDeserializer;
    private final byte[] serializedMessage;
    private transient Map<String, Serializable> context = new HashMap<String, Serializable>();
    private ObjectMapper objectMapper = null;
    public final static String DUMMY_CREATOR_USER_ID = "creatorUserId";
    private String creatorUserId = DUMMY_CREATOR_USER_ID;

    private Map<String, String> extraCustomHeaders = new HashMap<String, String>();

    public void setPrefixedExtraCustomHeaders(Map<String, String> prefixedCustomHeaders) {
        extraCustomHeaders = stripCustomHeadersPrefix(prefixedCustomHeaders);
    }

    public void setExtraCustomHeaders(Map<String, String> customHeaders) {
        for (Map.Entry<String, String> entry : customHeaders.entrySet()) {
            if (entry.getKey().startsWith(Message.CUSTOM_HEADER_PREFIX)) {
                logger.error("Extra custom header must not be prefixed: {} {}!", entry.getKey(), entry.getValue());
                throw new JoynrIllegalStateException("Extra custom header must not be prefixed: " + entry.getKey() + " "
                        + entry.getValue() + " !");
            }
        }
        this.extraCustomHeaders = new HashMap<>(customHeaders);
    }

    public Map<String, String> getExtraCustomHeaders() {
        return (extraCustomHeaders != null) ? new HashMap<>(extraCustomHeaders) : null;
    }

    public ImmutableMessage(byte[] serializedMessage) throws EncodingException, UnsuppportedVersionException {
        this.serializedMessage = serializedMessage.clone();
        messageDeserializer = new MessageDeserializerImpl(this.serializedMessage);
    }

    @JsonIgnore
    public byte[] getSerializedMessage() {
        return serializedMessage.clone();
    }

    public long getTtlMs() {
        return messageDeserializer.getTtlMs();
    }

    public boolean isTtlAbsolute() {
        return messageDeserializer.isTtlAbsolute();
    }

    public String getSender() {
        return messageDeserializer.getSender();
    }

    public String getRecipient() {
        return messageDeserializer.getRecipient();
    }

    public String getReplyTo() {
        return messageDeserializer.getHeader(Message.HEADER_REPLY_TO);
    }

    public Message.MessageType getType() {
        return Message.MessageType.fromString((messageDeserializer.getHeader(Message.HEADER_MSG_TYPE)));
    }

    public String getEffort() {
        return messageDeserializer.getHeader(Message.HEADER_EFFORT);
    }

    public void setCreatorUserId(String userId) {
        this.creatorUserId = userId;
    }

    public String getCreatorUserId() {
        return creatorUserId;
    }

    public byte[] getUnencryptedBody() throws EncodingException {
        return messageDeserializer.getUnencryptedBody();
    }

    public Map<String, String> getHeaders() {
        return messageDeserializer.getHeaders();
    }

    public Map<String, String> getCustomHeaders() {
        return stripCustomHeadersPrefix(messageDeserializer.getHeaders());
    }

    private Map<String, String> stripCustomHeadersPrefix(Map<String, String> prefixedCustomHeaders) {
        Map<String, String> customHeaders = new HashMap<>();
        for (Map.Entry<String, String> entry : prefixedCustomHeaders.entrySet()) {
            if (entry.getKey().startsWith(Message.CUSTOM_HEADER_PREFIX)) {
                String key = entry.getKey().replaceFirst("^" + Message.CUSTOM_HEADER_PREFIX, "");
                customHeaders.put(key, entry.getValue());
            }
        }
        return customHeaders;
    }

    public Map<String, String> getPrefixedCustomHeaders() {
        Map<String, String> prefixedCustomHeaders = new HashMap<>();
        for (Map.Entry<String, String> entry : getHeaders().entrySet()) {
            if (entry.getKey().startsWith(Message.CUSTOM_HEADER_PREFIX)) {
                prefixedCustomHeaders.put(entry.getKey(), entry.getValue());
            }
        }
        return prefixedCustomHeaders;
    }

    public void setContext(HashMap<String, Serializable> context) {
        this.context = (context != null) ? new HashMap<>(context) : null;
    }

    public Map<String, Serializable> getContext() {
        return (context != null) ? new HashMap<>(context) : null;
    }

    public void setReceivedFromGlobal(boolean receivedFromGlobal) {
        this.receivedFromGlobal = receivedFromGlobal;
    }

    public boolean isReceivedFromGlobal() {
        return receivedFromGlobal;
    }

    public void messageProcessed() {
        isMessageProcessed = true;
    }

    public boolean isMessageProcessed() {
        return isMessageProcessed;
    }

    public String getId() {
        return messageDeserializer.getHeader(Message.HEADER_ID);
    }

    public boolean isEncrypted() {
        return messageDeserializer.isEncrypted();
    }

    public boolean isSigned() {
        return messageDeserializer.isSigned();
    }

    public boolean isCompressed() {
        return messageDeserializer.isCompressed();
    }

    @JsonIgnore
    public int getMessageSize() {
        return messageDeserializer.getMessageSize();
    }

    @JsonIgnore
    public String toLogMessage() {
        if (objectMapper == null) {
            objectMapper = new ObjectMapper();

            SimpleModule module = new SimpleModule();
            module.addSerializer(byte[].class, new PayloadSerializer(byte[].class));
            module.addSerializer(MessageType.class, new TypeSerializer(MessageType.class));
            objectMapper.registerModule(module);
        }

        try {
            return objectMapper.writeValueAsString(this);
        } catch (JsonProcessingException e) {
            return "";
        }
    }

    @Override
    public String toString() {
        return toLogMessage();
    }

    private static class PayloadSerializer extends StdSerializer<byte[]> {
        private static final long serialVersionUID = 1L;

        protected PayloadSerializer(Class<byte[]> t) {
            super(t);
        }

        @Override
        public void serialize(byte[] value, JsonGenerator gen, SerializerProvider provider) throws IOException {
            gen.writeString(new String(value, StandardCharsets.UTF_8));
        }
    }

    private static class TypeSerializer extends StdSerializer<MessageType> {
        private static final long serialVersionUID = 1L;

        protected TypeSerializer(Class<MessageType> t) {
            super(t);
        }

        @Override
        public void serialize(MessageType value, JsonGenerator gen, SerializerProvider provider) throws IOException {
            if (value == null) {
                gen.writeString("null");
            } else {
                gen.writeString(value.toString());
            }
        }
    }

    public boolean isReply() {
        final Message.MessageType messageType = getType();
        return MessageType.VALUE_MESSAGE_TYPE_REPLY.equals(messageType)
                || MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY.equals(messageType);
    }

    public String getTrackingInfo() {
        String requestReplyId = getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
        StringBuilder trackingInfo = new StringBuilder(256).append("messageId: ")
                                                           .append(getId())
                                                           .append(", type: ")
                                                           .append(getType())
                                                           .append(", sender: ")
                                                           .append(getSender())
                                                           .append(", recipient: ")
                                                           .append(getRecipient());
        if (requestReplyId != null) {
            trackingInfo.append(", requestReplyId: ").append(requestReplyId);
        }
        trackingInfo.append(", expiryDate: ").append(getTtlMs()).append(", size: ").append(getMessageSize());
        return trackingInfo.toString();
    }

}
