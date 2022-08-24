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
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.fasterxml.jackson.core.JacksonException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.exceptions.MethodInvocationException;
import joynr.types.Version;

public class MethodInvocationExceptionDeserializer extends StdDeserializer<MethodInvocationException> {
    private static final long serialVersionUID = 1L;

    private static final String JSON_FIELD_NAME_VERSION = "providerVersion";
    private static Class<? extends JoynrRuntimeException> targetType = MethodInvocationException.class;
    private static Set<String> expectedFields = Stream.of(JoynrException.JSON_FIELD_NAME_MESSAGE,
                                                          JSON_FIELD_NAME_VERSION)
                                                      .collect(Collectors.toCollection(HashSet::new));;

    public MethodInvocationExceptionDeserializer() {
        super(MethodInvocationException.class);
    }

    @Override
    public MethodInvocationException deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException,
                                                                                             JacksonException {
        String message = null;
        Version version = null;

        JsonNode tree = ctxt.readTree(jp);

        JoynrExceptionDeserializationUtils.validate(jp, tree, expectedFields, targetType);

        message = JoynrExceptionDeserializationUtils.deserializeMessage(tree);

        JsonNode versionNode = tree.findValue(JSON_FIELD_NAME_VERSION);
        if (versionNode != null) {
            version = JoynrExceptionDeserializationUtils.getObjectMapper().readValue(versionNode.toString(),
                                                                                     Version.class);
        }

        return new MethodInvocationException(message, version);
    }

}
