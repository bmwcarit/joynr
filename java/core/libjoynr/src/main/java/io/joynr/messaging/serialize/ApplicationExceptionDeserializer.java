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
import com.fasterxml.jackson.databind.exc.InvalidFormatException;

import io.joynr.exceptions.JoynrException;
import joynr.exceptions.ApplicationException;

public class ApplicationExceptionDeserializer extends StdDeserializer<ApplicationException> {
    private static final long serialVersionUID = 1L;

    static final String JSON_FIELD_NAME_ERROR = "error";
    static final String JSON_FIELD_NAME_ERROR_NAME = "name";

    private static Set<String> expectedFields = Stream.of(JoynrException.JSON_FIELD_NAME_MESSAGE, JSON_FIELD_NAME_ERROR)
                                                      .collect(Collectors.toCollection(HashSet::new));

    public ApplicationExceptionDeserializer() {
        super(ApplicationException.class);
    }

    public Enum<?> deserializError(JsonNode tree) throws IOException {
        String typeName = null;
        String enumName = null;

        JsonNode typeNameNode = tree.findValue(JoynrException.JSON_FIELD_NAME_TYPE);
        if (typeNameNode == null) {
            throw new IOException("Invalid Json format: \"" + JoynrException.JSON_FIELD_NAME_TYPE + "\" not found!");
        }
        typeName = typeNameNode.asText();

        JsonNode enumNameNode = tree.findValue(JSON_FIELD_NAME_ERROR_NAME);
        if (enumNameNode == null) {
            throw new IOException("Invalid Json format: enum \"" + JSON_FIELD_NAME_ERROR_NAME + "\" not found!");
        }
        enumName = enumNameNode.asText();

        try {
            return Enum.valueOf(Class.forName(typeName).asSubclass(Enum.class), enumName);
        } catch (ClassNotFoundException e) {
            try {
                int indexOfLastDot = typeName.lastIndexOf(".");
                typeName = new StringBuilder(typeName).replace(indexOfLastDot, indexOfLastDot + 1, "$").toString();
                return Enum.valueOf(Class.forName(typeName).asSubclass(Enum.class), enumName);
            } catch (ClassNotFoundException e2) {
                throw new IOException(e2);
            }
        }
    }

    @Override
    public ApplicationException deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException,
                                                                                        JacksonException {
        String message = null;
        Enum<?> error = null;

        JsonNode tree = ctxt.readTree(jp);

        JoynrExceptionDeserializationUtils.validate(jp, tree, expectedFields, ApplicationException.class);

        message = JoynrExceptionDeserializationUtils.deserializeMessage(tree);

        JsonNode errorNode = tree.findValue(JSON_FIELD_NAME_ERROR);
        if (errorNode == null) {
            throw new InvalidFormatException(jp,
                                             "Invalid Json format: enum " + JSON_FIELD_NAME_ERROR + " not found!",
                                             JSON_FIELD_NAME_ERROR,
                                             ApplicationException.class);
        }

        error = deserializError(errorNode);

        return new ApplicationException(error, message, this);
    }
}
