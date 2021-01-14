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
package io.joynr.messaging;

import java.io.Serializable;
import java.util.Map;
import java.util.Optional;

public abstract class RawMessagingPreprocessor {

    /**
     * @param rawMessage the raw joynr message bytes
     * @param context you can optionally add String:Serializable pairs to this map, which
     * will be made available to the message's recipient. The value must be serializable to
     * support persistence of messages that could not be processed in the current lifecycle.
     * The map is preinitialized.
     * @return the processed rawMessage. NOTE: you are responsible for
     * returning a message that can still be parsed by the messaging skeleton;
     * otherwise the message will be discarded.
     */
    public abstract byte[] process(byte[] rawMessage, Optional<Map<String, Serializable>> context);

}
