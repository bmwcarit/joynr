package io.joynr.messaging.datatypes;

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

public enum JoynrMessagingErrorCode {

    /**
     * Enum codes plus offset
     */
    JOYNRMESSAGINGERROR_CHANNELNOTFOUND(1, "Channel not found"), JOYNRMESSAGINGERROR_CHANNELNOTSET(2, "Channel not set"), JOYNRMESSAGINGERROR_EXPIRYDATENOTSET(
            3, "TTL not set"), JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED(4, "TTL expired"), JOYNRMESSAGINGERROR_INVALIDMESSAGE(
            5, "Invalid message"), JOYNRMESSAGINGERROR_TRACKINGIDNOTSET(6, "Atmosphere Tracking Id not set");

    private static final int OFFSET = 10000;

    private static Map<Integer, JoynrMessagingErrorCode> codeToErrorCode;
    private int code;
    private String description;

    static {
        codeToErrorCode = new HashMap<Integer, JoynrMessagingErrorCode>();
        for (JoynrMessagingErrorCode errorCode : values()) {
            codeToErrorCode.put(errorCode.code, errorCode);
        }
    }

    private JoynrMessagingErrorCode(int code, String description) {
        this.code = OFFSET + code;
        this.description = description;
    }

    public static JoynrMessagingErrorCode getJoynMessagingErrorCode(int code) {
        return codeToErrorCode.get(code);
    }

    public int getCode() {
        return code;
    }

    public String getDescription() {
        return description;
    }

    public static void main(String[] args) {
        System.out.println(JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTFOUND);
        System.out.println(JoynrMessagingErrorCode.getJoynMessagingErrorCode(OFFSET + 3));
    }
}