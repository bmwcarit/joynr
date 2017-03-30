package io.joynr.messaging.datatypes;

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

import com.fasterxml.jackson.annotation.JsonTypeInfo;
import com.fasterxml.jackson.annotation.JsonTypeName;

@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, include = JsonTypeInfo.As.PROPERTY, property = "_typeName")
@JsonTypeName(value = "Error")
public class JoynrMessagingError {

    /**
     * The error code
     */
    private int code;

    /**
     * the reason the error occurred, in english text
     */

    private String reason;

    protected JoynrMessagingError() {
    }

    public JoynrMessagingError(int code, String reason) {
        this.code = code;
        this.reason = reason;

    }

    /**
     * Copy Constructor
     * 
     * @param error The JoynrMessagingError object to be copied from
     */
    public JoynrMessagingError(JoynrMessagingError error) {
        this.code = error.code;
        this.reason = error.reason;
    }

    /**
     * Overriden to provide string represention of the message object for Atmosphere used in response.
     * 
     * @return the string representation of the message object
     */
    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("\r\ncode: ");
        stringBuilder.append(getCode());
        stringBuilder.append("\r\nreason: ");
        stringBuilder.append(getReason());
        stringBuilder.append("\r\n");
        return stringBuilder.toString();
    }

    public int getCode() {
        return code;
    }

    public String getReason() {
        return reason;
    }

}
