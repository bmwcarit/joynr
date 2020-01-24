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

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

/**
 * Contains a global mapping of integer error code to a implementation of
 * {@link JoynrErrorCode}.<br>
 * This class contains a global map of all error codes used in the system. Each
 * implementation of {@link JoynrErrorCode} is expected to register its error
 * codes in here. Possible conflicts of integer error codes will be detected
 * here.
 * 
 */
public class JoynrErrorCodeMapper {

    private static Map<Integer, JoynrErrorCode> codeToErrorCode = new HashMap<Integer, JoynrErrorCode>();

    /**
     * Stores the mapping of integer error codes as returned by
     * {@link JoynrErrorCode#getCode()} to {@link JoynrErrorCode} objects.
     * 
     * @param errorCode
     *            the error code to store a mapping for
     * @throws IllegalArgumentException
     *             if there's already a {@link JoynrErrorCode} stored with the
     *             same integer code.
     */
    public static void storeErrorCodeMapping(JoynrErrorCode errorCode) {

        if (codeToErrorCode.containsKey(errorCode.getCode())) {
            JoynrErrorCode conflictingErrorCode = codeToErrorCode.get(errorCode.getCode());

            if (errorCode != conflictingErrorCode) {

                throw new IllegalArgumentException("Error code '" + errorCode.getCode() + ":" + errorCode.toString()
                        + " (" + errorCode.getDescription() + ")' conflicts with error code for '"
                        + conflictingErrorCode.toString() + " (" + errorCode.getDescription() + ")'");
            }
        }

        codeToErrorCode.put(errorCode.getCode(), errorCode);
    }

    /**
     * Returns the {@link JoynrErrorCode} object that has the given error code.
     * 
     * @param code
     *            the integer error code
     * @return an Optional containing the {@link JoynrErrorCode} object for which
     *         {@link JoynrErrorCode#getCode()} matches the given integer error
     *         code. If there's no error code object for that error code,
     *         an empty Optional is returned.
     */
    public static Optional<JoynrErrorCode> getErrorCode(int code) {
        return Optional.ofNullable(codeToErrorCode.get(code));
    }
}
