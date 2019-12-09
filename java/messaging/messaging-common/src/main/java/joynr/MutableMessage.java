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

import static io.joynr.util.JoynrUtil.createUuidString;

import java.util.HashMap;
import java.util.Map;

import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;

import io.joynr.smrf.EncodingException;
import io.joynr.smrf.MessageSerializer;
import io.joynr.smrf.MessageSerializerImpl;
import io.joynr.smrf.UnsuppportedVersionException;

/**
 * Represents a joynr message which is about to be transmitted. It will be converted to
 * an {@link ImmutableMessage} before it is passed to a sub-class of
 * {@link io.joynr.messaging.routing.AbstractMessageRouter}.
 */
public class MutableMessage extends Message {
    private String sender;
    private String recipient;
    private long ttlMs;
    private boolean ttlAbsolute;
    private byte[] payload;

    private String id;
    private MessageType type;
    private String replyTo;
    private String effort;
    private Map<String, String> customHeaders = new HashMap<>();

    private transient boolean compressed = false;
    private transient boolean statelessAsync;

    public MutableMessage() {
        id = createUuidString();
    }

    public ImmutableMessage getImmutableMessage() throws SecurityException, EncodingException,
                                                  UnsuppportedVersionException {
        MessageSerializer messageSerializer = new MessageSerializerImpl();

        messageSerializer.setSender(getSender());
        messageSerializer.setRecipient(getRecipient());
        messageSerializer.setTtlMs(getTtlMs());
        messageSerializer.setTtlAbsolute(isTtlAbsolute());
        messageSerializer.setHeaders(createHeader());
        messageSerializer.setBody(getPayload());
        messageSerializer.setCompressed(compressed);

        return new ImmutableMessage(messageSerializer.serialize());
    }

    private Map<String, String> createHeader() {
        Map<String, String> header = new HashMap<>();

        if (customHeaders != null && !customHeaders.isEmpty()) {
            for (Map.Entry<String, String> entry : customHeaders.entrySet()) {
                header.put(CUSTOM_HEADER_PREFIX + entry.getKey(), entry.getValue());
            }
        }
        if (type != null) {
            putIfValueNotNull(Message.HEADER_MSG_TYPE, type.toString(), header);
        }
        putIfValueNotNull(Message.HEADER_ID, id, header);
        putIfValueNotNull(Message.HEADER_REPLY_TO, replyTo, header);
        putIfValueNotNull(Message.HEADER_EFFORT, effort, header);

        return header;
    }

    private void putIfValueNotNull(String key, String value, Map<String, String> destination) {
        if (value != null) {
            destination.put(key, value);
        }
    }

    public String getSender() {
        return sender;
    }

    public void setSender(String sender) {
        this.sender = sender;
    }

    public String getRecipient() {
        return recipient;
    }

    public void setRecipient(String recipient) {
        this.recipient = recipient;
    }

    public long getTtlMs() {
        return ttlMs;
    }

    public void setTtlMs(long ttlMs) {
        this.ttlMs = ttlMs;
    }

    public boolean isTtlAbsolute() {
        return ttlAbsolute;
    }

    public void setTtlAbsolute(boolean ttlAbsolute) {
        this.ttlAbsolute = ttlAbsolute;
    }

    public MessageType getType() {
        return type;
    }

    public void setType(MessageType type) {
        this.type = type;
    }

    public String getId() {
        return id;
    }

    // Findbugs wants us to return a copy. However, this is a getter for the externally provided
    // payload which may change at any time.
    @SuppressFBWarnings("EI_EXPOSE_REP")
    public byte[] getPayload() {
        return payload;
    }

    // Findbugs wants us to copy the parameter. However, the payload is only used during the serialization.
    // Therefore it is okay if it changes.
    @SuppressFBWarnings("EI_EXPOSE_REP")
    public void setPayload(byte[] payload) {
        this.payload = payload;
    }

    public String getReplyTo() {
        return replyTo;
    }

    public void setReplyTo(String replyTo) {
        this.replyTo = replyTo;
    }

    public String getEffort() {
        return effort;
    }

    public void setEffort(String effort) {
        this.effort = effort;
    }

    public Map<String, String> getCustomHeaders() {
        return customHeaders;
    }

    public void setCustomHeaders(Map<String, String> customHeaders) {
        this.customHeaders.putAll(customHeaders);
    }

    public void setCompressed(boolean compressed) {
        this.compressed = compressed;
    }

    public boolean getCompressed() {
        return this.compressed;
    }

    public boolean isStatelessAsync() {
        return statelessAsync;
    }

    public void setStatelessAsync(boolean statelessAsync) {
        this.statelessAsync = statelessAsync;
    }
}
