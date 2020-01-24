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
package io.joynr.messaging.datatypes;

import java.util.Optional;

/**
 * Error codes related to messaging between joynr participants.
 * 
 */
public enum JoynrMessagingErrorCode implements JoynrErrorCode {

    /**
     * Enum codes plus offset
     */
    JOYNRMESSAGINGERROR_CHANNELNOTFOUND(1, "Channel not found"), //
    JOYNRMESSAGINGERROR_CHANNELNOTSET(2, "Channel not set"), //
    JOYNRMESSAGINGERROR_EXPIRYDATENOTSET(3, "TTL not set"), //
    JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED(4, "TTL expired"), //
    JOYNRMESSAGINGERROR_INVALIDMESSAGE(5, "Invalid message"), //
    JOYNRMESSAGINGERROR_TRACKINGIDNOTSET(6, "Atmosphere Tracking Id not set"), //
    JOYNRMESSAGINGERROR_SESSIONIDSET(7, "Session Id set"), //
    JOYNRMESSAGINGERROR_SESSIONIDNOTSET(8, "Session Id not set"), //
    JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED(9,
            "Message deserialization failed"), JOYNRMESSAGINGERROR_RELATIVE_TTL_UNSPORTED(10,
                    "Relative TTL is not supported"), JOYNRMESSAGINGERROR_UNDEFINED(0, "Undefined error");

    private static final int OFFSET = 10000;

    private int code;
    private String description;

    private JoynrMessagingErrorCode(int code, String description) {
        this.code = OFFSET + code;
        this.description = description;
        JoynrErrorCodeMapper.storeErrorCodeMapping(this);
    }

    /**
     * Creates a {@link JoynrMessagingErrorCode} object from an integer error
     * code.
     * 
     * @param code
     *            error code as integer
     * @return the matching {@link JoynrMessagingErrorCode} or
     *         {@link JoynrMessagingErrorCode#JOYNRMESSAGINGERROR_UNDEFINED}, if
     *         there's no matching code.
     */
    public static JoynrMessagingErrorCode getJoynrMessagingErrorCode(int code) {
        Optional<JoynrErrorCode> errorCode = JoynrErrorCodeMapper.getErrorCode(code);

        if (!errorCode.isPresent() || !(errorCode.get() instanceof JoynrMessagingErrorCode)) {
            return JOYNRMESSAGINGERROR_UNDEFINED;
        }

        return (JoynrMessagingErrorCode) errorCode.get();
    }

    @Override
    public int getCode() {
        return code;
    }

    @Override
    public String getDescription() {
        return description;
    }

    public static void main(String[] args) {
        System.out.println(JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTFOUND);
        System.out.println(JoynrMessagingErrorCode.getJoynrMessagingErrorCode(OFFSET + 3));
    }
}
