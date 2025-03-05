/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2019 BMW Car IT GmbH
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
package io.joynr.messaging;

import static io.joynr.messaging.MessagingQosEffort.NORMAL;

import java.util.HashMap;
import java.util.Map;

public class MessagingQos {
    public static final int DEFAULT_TTL = 60000;
    public static final Map<String, String> DEFAULTQOS = new HashMap<String, String>();
    private long ttl_ms;
    private Map<String, String> customHeaders = new HashMap<>();
    private MessagingQosEffort effort;
    private boolean encrypt;
    private boolean compress = false;

    public MessagingQos(MessagingQos src) {
        ttl_ms = src.getRoundTripTtl_ms();
        effort = src.getEffort();
        encrypt = src.getEncrypt();
        compress = src.getCompress();
        customHeaders = (src.customHeaders != null) ? new HashMap<>(src.customHeaders) : null;
    }

    /**
     * MessagingQos with default values
     */
    public MessagingQos() {
        this(DEFAULT_TTL);
    }

    /**
     * @param ttl_ms Roundtrip timeout for rpc requests.
     */
    public MessagingQos(long ttl_ms) {
        this(ttl_ms, NORMAL);
    }

    /**
     * @param ttl_ms Roundtrip timeout for rpc requests.
     * @param effort the effort to expend in ensuring message delivery.
     */
    public MessagingQos(long ttl_ms, MessagingQosEffort effort) {
        this(ttl_ms, effort, false);
    }

    /**
     * @param effort the effort to expend in ensuring message delivery.
     */
    public MessagingQos(MessagingQosEffort effort) {
        this(DEFAULT_TTL, effort, false);
    }

    /**
     * @param effort the effort to expend in ensuring message delivery.
     * @param encrypt specifies, whether messages will be sent encrypted
     */
    public MessagingQos(MessagingQosEffort effort, boolean encrypt) {
        this(DEFAULT_TTL, effort, encrypt);
    }

    /**
     * @param encrypt specifies, whether messages will be sent encrypted
     */
    public MessagingQos(boolean encrypt) {
        this(DEFAULT_TTL, encrypt);
    }

    /**
     * @param ttl_ms Roundtrip timeout for rpc requests.
     * @param encrypt specifies, whether messages will be sent encrypted
     */
    public MessagingQos(long ttl_ms, boolean encrypt) {
        this(ttl_ms, NORMAL, encrypt);
    }

    /**
     * @param ttl_ms Roundtrip timeout for rpc requests.
     * @param effort the effort to expend in ensuring message delivery.
     * @param encrypt specifies, whether messages will be sent encrypted
     */
    public MessagingQos(long ttl_ms, MessagingQosEffort effort, boolean encrypt) {
        this.ttl_ms = ttl_ms;
        this.effort = effort;
        this.encrypt = encrypt;
    }

    /**
     * @param ttl_ms Roundtrip timeout for rpc requests.
     * @param effort the effort to expend in ensuring message delivery.
     * @param customHeaders map containing custom headers. <br>
     *                      Keys may contain ascii alphanumeric or hyphen. <br>
     *                      Values may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question
     *                      mark, hyphen, dot, star, forward slash and back slash.
     * @param compress specifies, whether messages will be sent compressed
     */
    public MessagingQos(long ttl_ms, MessagingQosEffort effort, Map<String, String> customHeaders, boolean compress) {
        this.ttl_ms = ttl_ms;
        this.effort = effort;
        this.customHeaders = (customHeaders != null) ? new HashMap<>(customHeaders) : null;
        this.compress = compress;
    }

    public long getRoundTripTtl_ms() {
        return ttl_ms;
    }

    /**
     * @param ttl_ms Time to live for a joynr message and the corresponding answer on the complete way from the sender to
     *               the receiver and back.
     */
    public void setTtl_ms(final long ttl_ms) {
        this.ttl_ms = ttl_ms;
    }

    public MessagingQosEffort getEffort() {
        return effort;
    }

    public void setEffort(MessagingQosEffort effort) {
        this.effort = effort;
    }

    /**
     * Gets Encrypt
     *
     * @return specifies, whether messages will be sent encrypted
     */
    public boolean getEncrypt() {
        return encrypt;
    }

    /**
     * @param encrypt specifies, whether messages will be sent encrypted
     */
    public void setEncrypt(boolean encrypt) {
        this.encrypt = encrypt;
    }

    /**
     * Gets Compress
     *
     * @return specifies, whether messages will be sent compressed
     */
    public boolean getCompress() {
        return compress;
    }

    /**
     * @param compress specifies, whether messages will be sent compressed
     */
    public void setCompress(boolean compress) {
        this.compress = compress;
    }

    /**
     * @param key   may contain ascii alphanumeric or hyphen.
     * @param value may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark, hyphen,
     *              dot, star, forward slash and back slash.
     * @throws IllegalArgumentException if key or value contain any illegal characters
     */
    public void putCustomMessageHeader(String key, String value) {
        checkKeyAndValue(key, value);
        customHeaders.put(key, value);
    }

    /**
     * @param newCustomHeaders map containing custom headers. <br>
     *                         Keys may contain ascii alphanumeric or hyphen. <br>
     *                         Values may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark,
     *                         hyphen, dot, star, forward slash and back slash.
     * @throws IllegalArgumentException if key or value contain any illegal characters
     */
    public void putAllCustomMessageHeaders(Map<String, String> newCustomHeaders) {
        for (Map.Entry<String, String> entry : newCustomHeaders.entrySet()) {
            checkKeyAndValue(entry.getKey(), entry.getValue());
        }

        customHeaders.putAll(newCustomHeaders);
    }

    /**
     * @param key   may contain ascii alphanumeric or hyphen.
     * @param value may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark, hyphen,
     *              dot, star, forward slash and back slash.
     */
    private void checkKeyAndValue(String key, String value) {
        String keyPattern = "^[a-zA-Z0-9\\-]*$";
        String valuePattern = "^[a-zA-Z0-9 ;:,+&\\?\\-\\.\\*\\/\\\\_]*$";
        if (!key.matches(keyPattern)) {
            throw new IllegalArgumentException("key may only contain alphanumeric characters");
        }
        if (!value.matches(valuePattern)) {
            throw new IllegalArgumentException("value contains illegal character. See JavaDoc for allowed characters");
        }
    }

    public Map<String, String> getCustomMessageHeaders() {
        return (customHeaders != null) ? new HashMap<>(customHeaders) : null;
    }

    @Override
    public String toString() {
        return "MessagingQos [" + "compress=" + this.compress + ", " + "customHeaders=" + this.customHeaders.toString()
                + ", " + "effort=" + this.effort.toString() + ", " + "encrypt=" + this.encrypt + ", " + "ttl_ms="
                + this.ttl_ms + "]";
    }

    /**
     * Calculate code for hashing based on member contents
     *
     * @return The calculated hash code
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (ttl_ms ^ (ttl_ms >>> 32));
        result = prime * result + ((effort == null) ? 0 : effort.hashCode());
        result = prime * result + ((customHeaders == null) ? 0 : customHeaders.hashCode());
        result = prime * result + Boolean.hashCode(encrypt);
        result = prime * result + Boolean.hashCode(compress);
        return result;
    }

    /**
     * Check for equality
     *
     * @param obj Reference to the object to compare to
     * @return true, if objects are equal, false otherwise
     */
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
        MessagingQos other = (MessagingQos) obj;
        if (ttl_ms != other.ttl_ms) {
            return false;
        }
        if (effort != other.effort) {
            return false;
        }
        if (encrypt != other.encrypt) {
            return false;
        }
        if (compress != other.compress) {
            return false;
        }
        if (!customHeaders.equals(other.customHeaders)) {
            return false;
        }
        return true;
    }
}
