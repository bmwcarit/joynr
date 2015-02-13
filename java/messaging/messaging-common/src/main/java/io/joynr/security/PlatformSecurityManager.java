package io.joynr.security;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import joynr.JoynrMessage;

public interface PlatformSecurityManager {
    /**
     * @return the platform user ID of the running Java process.
     */
    String getCurrentProcessUserId();

    /**
     * @param message
     * @return signed JoynrMessage
     */
    JoynrMessage sign(JoynrMessage message);

    /**
     * @param message
     * @return if message is valid returns true
     */
    boolean validate(JoynrMessage message);

    /**
     * @param message
     * @return encrypted JoynrMessage
     */
    JoynrMessage encrypt(JoynrMessage message);

    /**
     * @param message
     * @return decrypted JoynrMessage
     */
    JoynrMessage decrypt(JoynrMessage message);
}
