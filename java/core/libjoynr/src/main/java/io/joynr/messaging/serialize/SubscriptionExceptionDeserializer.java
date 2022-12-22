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
import io.joynr.exceptions.SubscriptionException;

public class SubscriptionExceptionDeserializer extends StdDeserializer<SubscriptionException> {
    private static final long serialVersionUID = 1L;
    private static final String JSON_FIELD_NAME_SUBSCRIPTIONID = "subscriptionId";

    private static Class<? extends JoynrRuntimeException> targetType = SubscriptionException.class;
    private static Set<String> expectedFields = Stream.of(JoynrException.JSON_FIELD_NAME_MESSAGE,
                                                          JSON_FIELD_NAME_SUBSCRIPTIONID)
                                                      .collect(Collectors.toCollection(HashSet::new));

    public SubscriptionExceptionDeserializer() {
        super(SubscriptionException.class);
    }

    @Override
    public SubscriptionException deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException,
                                                                                         JacksonException {
        String message = null;
        String subscriptionId = null;

        JsonNode tree = ctxt.readTree(jp);

        JoynrExceptionDeserializationUtils.validate(jp, tree, expectedFields, targetType);

        message = JoynrExceptionDeserializationUtils.deserializeMessage(tree);

        JsonNode subscriptionIdNode = tree.findValue(JSON_FIELD_NAME_SUBSCRIPTIONID);
        if (subscriptionIdNode == null) {
            throw new IOException("Invalid Json format: " + JSON_FIELD_NAME_SUBSCRIPTIONID + " not found!");
        }
        subscriptionId = JoynrExceptionDeserializationUtils.getObjectMapper().readValue(subscriptionIdNode.toString(),
                                                                                        String.class);

        return new SubscriptionException(subscriptionId, message);
    }
}
