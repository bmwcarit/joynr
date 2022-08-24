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

import java.util.Iterator;
import java.util.Set;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.exc.InvalidFormatException;
import com.fasterxml.jackson.databind.exc.ValueInstantiationException;
import com.fasterxml.jackson.databind.util.ClassUtil;
import com.google.inject.Inject;

import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.util.ObjectMapper;

public class JoynrExceptionDeserializationUtils {

    @Inject
    private static ObjectMapper objectMapper;

    private JoynrExceptionDeserializationUtils() {
    }

    static ObjectMapper getObjectMapper() {
        return objectMapper;
    }

    static void validate(JsonParser jp,
                         JsonNode tree,
                         Set<String> expectedFields,
                         Class<? extends JoynrException> targetType) throws InvalidFormatException {
        Iterator<String> it = tree.fieldNames();
        while (it.hasNext()) {
            String field = it.next();
            if (!expectedFields.contains(field)) {
                throw new InvalidFormatException(jp,
                                                 "Invalid Json format: unexpected FIELD_NAME " + field,
                                                 field,
                                                 targetType);
            }
        }
    }

    static String deserializeMessage(JsonNode tree) {
        String message = null;
        JsonNode messageNode = tree.findValue(JoynrException.JSON_FIELD_NAME_MESSAGE);
        if (messageNode != null) {
            message = messageNode.asText();
        }
        return message;
    }

    static JoynrRuntimeException deserializeException(JsonParser jp,
                                                      JsonNode tree,
                                                      Class<? extends JoynrRuntimeException> targetType) throws JsonMappingException {
        String message = deserializeMessage(tree);

        try {
            JoynrRuntimeException e = targetType.getDeclaredConstructor(String.class).newInstance(message);
            return e;
        } catch (Exception e) {
            throw ValueInstantiationException.from(jp,
                                                   String.format("Cannot construct instance of %s: %s",
                                                                 ClassUtil.classOf(targetType),
                                                                 e));
        }
    }

}
