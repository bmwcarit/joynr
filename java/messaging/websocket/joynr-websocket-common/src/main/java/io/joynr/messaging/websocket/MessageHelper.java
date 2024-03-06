/*
 * #%L
 * %%
 * Copyright (C) 2024 BMW Car IT GmbH
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
package io.joynr.messaging.websocket;

import io.joynr.exceptions.JoynrRuntimeException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;

public class MessageHelper {

    private static final String ERR_EXTRACTION_FAILED = "Exception occurred during message extraction";

    private static final Logger logger = LoggerFactory.getLogger(MessageHelper.class);

    public static byte[] extractMessage(final byte[] payload, final int offset, final int length) {
        if (payload == null) {
            Exception exception = new NullPointerException("payload must not be null");
            throw new JoynrRuntimeException(ERR_EXTRACTION_FAILED, exception);
        }

        try {
            return Arrays.copyOfRange(payload, offset, offset + length);
        } catch (final ArrayIndexOutOfBoundsException | IllegalArgumentException exception) {
            logger.error(ERR_EXTRACTION_FAILED, exception);
            throw new JoynrRuntimeException(ERR_EXTRACTION_FAILED, exception);
        }
    }
}
