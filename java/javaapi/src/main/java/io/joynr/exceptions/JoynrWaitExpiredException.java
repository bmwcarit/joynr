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
package io.joynr.exceptions;

import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

public class JoynrWaitExpiredException extends JoynrTimeoutException {

    /**
     * DO NOT USE
     * Constructor for deserializer
     *
     * @param expiryDate expiry date
     * @param deserializer deserializer of the exception
     */
    public JoynrWaitExpiredException(long expiryDate, StdDeserializer<JoynrWaitExpiredException> deserializer) {
        super(expiryDate);
    }

    public JoynrWaitExpiredException() {
        super(System.currentTimeMillis());
    }

    private static final long serialVersionUID = -5357576996106391828L;

}
