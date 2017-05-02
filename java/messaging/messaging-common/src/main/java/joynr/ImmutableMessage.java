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

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.MessageDeserializer;
import io.joynr.smrf.MessageDeserializerImpl;
import io.joynr.smrf.UnsuppportedVersionException;

/**
 * This class wraps binary SMRF messages and provides access to the serialized attributes
 * of the message. This type is mainly used by the messaging layers and the message router.
 * {@link MutableMessage} objects will be converted to ImmutableMessage objects before they
 * are handed over to a sub-class of {@link io.joynr.messaging.routing.AbstractMessageRouter}.
 * It's not possible to modify a immutable message any more because the content may be signed.
 */
public class ImmutableMessage extends Message {
    private transient boolean receivedFromGlobal;
    private final MessageDeserializer messageDeserializer;
    private final byte[] serializedMessage;
    private transient Map<String, Serializable> context = new HashMap<String, Serializable>();

    // Findbugs doesn't want us to store a reference to the serialized message but to copy it.
    // However, this may have an impact on the performance.
    @SuppressFBWarnings("EI_EXPOSE_REP2")
    public ImmutableMessage(byte[] serializedMessage) throws EncodingException, UnsuppportedVersionException {
        messageDeserializer = new MessageDeserializerImpl(serializedMessage);
        this.serializedMessage = serializedMessage;
    }

    @SuppressFBWarnings("EI_EXPOSE_REP")
    public byte[] getSerializedMessage() {
        return serializedMessage;
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

    public String getType() {
        return messageDeserializer.getHeader(Message.HEADER_MSG_TYPE);
    }

    public String toLogMessage() {
        return "";
    }

    public String getEffort() {
        return messageDeserializer.getHeader(Message.HEADER_EFFORT);
    }

    public String getCreatorUserId() {
        return System.getProperty("user.name");
    }

    public byte[] getUnencryptedBody() throws EncodingException {
        return messageDeserializer.getUnencryptedBody();
    }

    public Map<String, String> getHeaders() {
        return messageDeserializer.getHeaders();
    }

    public Map<String, String> getCustomHeaders() {
        Map<String, String> customHeaders = new HashMap<>();
        for (Map.Entry<String, String> entry : getHeaders().entrySet()) {
            if (entry.getKey().startsWith(Message.CUSTOM_HEADER_PREFIX)) {
                String key = entry.getKey().replaceFirst("^" + Message.CUSTOM_HEADER_PREFIX, "");
                customHeaders.put(key, entry.getValue());
            }
        }
        return customHeaders;
    }

    public void setContext(HashMap<String, Serializable> context) {
        this.context = context;
    }

    public Map<String, Serializable> getContext() {
        return context;
    }

    public void setReceivedFromGlobal(boolean receivedFromGlobal) {
        this.receivedFromGlobal = receivedFromGlobal;
    }

    public boolean isReceivedFromGlobal() {
        return receivedFromGlobal;
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

    public int getMessageSize() {
        return messageDeserializer.getMessageSize();
    }
}
