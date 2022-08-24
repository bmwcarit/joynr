/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.messaging.serialize;

import java.io.IOException;
import java.util.Collections;
import java.util.Set;

import com.fasterxml.jackson.core.JacksonException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;

public class JoynrMessageNotSentExceptionDeserializer extends StdDeserializer<JoynrMessageNotSentException> {
    private static final long serialVersionUID = 1L;

    private static Class<? extends JoynrRuntimeException> targetType = JoynrMessageNotSentException.class;
    private static Set<String> expectedFields = Collections.singleton(JoynrException.JSON_FIELD_NAME_MESSAGE);

    public JoynrMessageNotSentExceptionDeserializer() {
        super(JoynrMessageNotSentException.class);
    }

    @Override
    public JoynrMessageNotSentException deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException,
                                                                                                JacksonException {
        JsonNode tree = ctxt.readTree(jp);

        JoynrExceptionDeserializationUtils.validate(jp, tree, expectedFields, targetType);

        return (JoynrMessageNotSentException) JoynrExceptionDeserializationUtils.deserializeException(jp,
                                                                                                      tree,
                                                                                                      targetType);
    }

}
