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

/**
 * Common interface for all error codes.<br>
 * 
 * Implementations of this interface are expected to be {@link Enum} values that
 * represent a certain type of error, e.g. messaging error types. The code range
 * (as returned by {@link JoynrErrorCode#getCode()}) should use a different
 * offset for each error type.<br>
 * All error codes should be stored in {@link JoynrErrorCodeMapper}. By storing
 * a mapping of integer error code to {@link JoynrErrorCode} via
 * {@link JoynrErrorCodeMapper#storeErrorCodeMapping(JoynrErrorCode)}, possible conflicts
 * of error codes will be detected.
 * 
 * @author christina.strobel
 * 
 */
public interface JoynrErrorCode {

    /**
     * Returns the code of the error as integer.
     * 
     * @return error code (including the offset of the type of error code)
     */
    public int getCode();

    /**
     * Returns a description of the error.
     * 
     * @return the description of the error.
     */
    public String getDescription();

}
