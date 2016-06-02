package io.joynr.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

public class MessagingQos {
	public static final int DEFAULT_TTL = 60000;
	public static final Map<String, String> DEFAULTQOS = new HashMap<String, String>();
	private long ttl_ms;
	private Map<String, String> customHeaders = new HashMap<>();

	/**
	 * MessagingQos with default values
	 */
	public MessagingQos() {
		ttl_ms = DEFAULT_TTL;
	}

	public MessagingQos(MessagingQos src) {
		ttl_ms = src.getRoundTripTtl_ms();
	}

	/**
	 * @param ttl_ms
	 *            Roundtrip timeout for rpc requests.
	 */
	public MessagingQos(long ttl_ms) {
		this.ttl_ms = ttl_ms;
	}

	public long getRoundTripTtl_ms() {
		return ttl_ms;
	}

	/**
	 * @param ttl_ms
	 *            Time to live for a joynr message and the corresponding answer on the complete way from the sender to
	 *            the receiver and back.
	 */
	public void setTtl_ms(final long ttl_ms) {
		this.ttl_ms = ttl_ms;
	}

    /**
     * @param key
     *            may contain ascii alphanumeric or hyphen.
     * @param value
     *            may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark, hyphen,
     *            dot, star, forward slash and back slash.
     * @Throws {@link IllegalArgumentException} if key or value contain any illegal characters
     */
    public void putCustomMessageHeader(String key, String value) {
        checkKeyAndValue(key, value);
        customHeaders.put(key, value);
    }

    /**
     *
     * @param newCustomHeaders
     *            map containing custom headers. <br>
     *            Keys may contain ascii alphanumeric or hyphen. <br>
     *            Values may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark,
     *            hyphen, dot, star, forward slash and back slash.
     * @Throws {@link IllegalArgumentException} if key or value contain any illegal characters
     */
    public void putAllCustomMessageHeaders(Map<String, String> newCustomHeaders) {
        for (Map.Entry<String, String> entry : newCustomHeaders.entrySet()) {
            checkKeyAndValue(entry.getKey(), entry.getValue());
        }

        customHeaders.putAll(newCustomHeaders);
    }

    /**
     *
     * @param key
     *            may contain ascii alphanumeric or hyphen.
     * @param value
     *            may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark, hyphen,
     *            dot, star, forward slash and back slash.
     */
	private void checkKeyAndValue(String key, String value) {
		String keyPattern = "^[a-zA-Z0-9\\-]*$";
		String valuePattern = "^[a-zA-Z0-9 ;:,+&\\?\\-\\.\\*\\/\\\\]*$";
		if (!key.matches(keyPattern)) {
			throw new IllegalArgumentException("key may only contain alphanumeric characters");
		}
		if (!value.matches(valuePattern)) {
			throw new IllegalArgumentException("value contains illegal character. See JavaDoc for allowed characters");
		}
	}

	public Map<String, String> getCustomMessageHeaders() {
		return customHeaders;
	}
}
