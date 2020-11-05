/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonNode;

import io.joynr.util.ObjectMapper;
import joynr.Request;

/**
 * Deserializer for Requests that uses the correct datatypes while deserializing.
 * This is especially necessary for enums, which are otherwise deserialized to String,
 * and arrays, which are otherwise deserialized to Object[]
 */
public class RequestDeserializer extends JsonDeserializer<Request> {
    private static final Logger logger = LoggerFactory.getLogger(RequestDeserializer.class);

    private ObjectMapper objectMapper;

    public RequestDeserializer(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    /* (non-Javadoc)
     * @see com.fasterxml.jackson.databind.JsonDeserializer#deserialize(com.fasterxml.jackson.core.JsonParser,
     * com.fasterxml.jackson.databind.DeserializationContext)
     */
    @Override
    public Request deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException, JsonProcessingException {
        JsonNode node = jp.getCodec().readTree(jp);
        String methodName = node.get("methodName").asText();
        String requestReplyId = node.get("requestReplyId").asText();

        ParamsAndParamDatatypesHolder paramsAndParamDatatypes = DeserializerUtils.deserializeParams(objectMapper,
                                                                                                    node,
                                                                                                    logger);

        return new Request(methodName,
                           paramsAndParamDatatypes.params,
                           paramsAndParamDatatypes.paramDatatypes,
                           requestReplyId);
    }
}
