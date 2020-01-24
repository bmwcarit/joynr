/*
 * #%L
 * joynr::java::messaging::service-common
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
 * Error codes related to controlled bounce proxy scenarios.
 * 
 * @author christina.strobel
 *
 */
public enum JoynrBounceProxyControlErrorCode implements JoynrErrorCode {

    BOUNCEPROXY_UNKNOWN(1, "Bounce Proxy unknown"), //
    BOUNCEPROXY_STATUS_UNKNOWN(2, "Bounce Proxy status unknown"), //
    BOUNCEPROXY_NOT_INITIALIZED(3, "Bounce Proxy not ready to handle requests"), //
    UNDEFINED(0, "Undefined error");

    private static final int OFFSET = 20000;

    private int code;
    private String description;

    private JoynrBounceProxyControlErrorCode(int code, String description) {
        this.code = OFFSET + code;
        this.description = description;
        JoynrErrorCodeMapper.storeErrorCodeMapping(this);
    }

    /**
     * Creates a {@link JoynrBounceProxyControlErrorCode} object from an integer error
     * code.
     * 
     * @param code
     *            error code as integer
     * @return the matching {@link JoynrBounceProxyControlErrorCode} or
     *         {@link JoynrBounceProxyControlErrorCode#UNDEFINED}, if
     *         there's no matching code.
     */
    public static JoynrBounceProxyControlErrorCode getJoynrBounceProxyControlErrorCode(int code) {
        Optional<JoynrErrorCode> errorCode = JoynrErrorCodeMapper.getErrorCode(code);

        if (!errorCode.isPresent() || !(errorCode.get() instanceof JoynrBounceProxyControlErrorCode)) {
            return JoynrBounceProxyControlErrorCode.UNDEFINED;
        }

        return (JoynrBounceProxyControlErrorCode) errorCode.get();
    }

    @Override
    public int getCode() {
        return code;
    }

    @Override
    public String getDescription() {
        return description;
    }

}
