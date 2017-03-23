package io.joynr.messaging.serialize;

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

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

import io.joynr.exceptions.JoynrSerializationException;
import io.joynr.messaging.JoynrMessageSerializer;
import joynr.JoynrMessage;

public class JsonSerializer implements JoynrMessageSerializer {

    private ObjectMapper objectMapper;

    @Inject
    public JsonSerializer(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    @Override
    public String serialize(JoynrMessage message) throws JoynrSerializationException {
        try {
            return objectMapper.writeValueAsString(message);
        } catch (JsonProcessingException e) {
            throw new JoynrSerializationException(e.getMessage());
        }
    }

    @Override
    public JoynrMessage deserialize(String serializedMessage) throws JoynrSerializationException {
        try {
            return objectMapper.readValue(serializedMessage, JoynrMessage.class);
        } catch (Exception e) {
            throw new JoynrSerializationException(e.getMessage());
        }
    }

}
