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

import java.io.IOException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;

import io.joynr.dispatcher.rpc.ReflectionUtils;
import joynr.Request;

/**
 * Deserializer for Requests that uses the correct datatype while deserializing.
 * This is especially necessary for arrays, which are otherwise deserialized to
 * Object[]
 *
 */
public class RequestDeserializer extends JsonDeserializer<Request> {
    private static final Logger logger = LoggerFactory.getLogger(RequestDeserializer.class);

    private ObjectMapper objectMapper;

    public RequestDeserializer(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    /* (non-Javadoc)
     * @see com.fasterxml.jackson.databind.JsonDeserializer#deserialize(com.fasterxml.jackson.core.JsonParser, com.fasterxml.jackson.databind.DeserializationContext)
     */
    @Override
    public Request deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException, JsonProcessingException {

        JsonNode node = jp.getCodec().readTree(jp);
        String methodName = node.get("methodName").asText();
        String requestReplyId = node.get("requestReplyId").asText();
        String[] paramDatatypes;
        Object[] params;
        Class<?>[] javaClasses = null;

        ArrayNode paramDatatypesNode = (ArrayNode) node.get("paramDatatypes");
        if (paramDatatypesNode != null) {
            paramDatatypes = new String[paramDatatypesNode.size()];
            int i = 0;
            for (final JsonNode paramDatatypeNode : paramDatatypesNode) {
                paramDatatypes[i] = paramDatatypeNode.asText();
                i++;
            }
            javaClasses = ReflectionUtils.toJavaClasses(paramDatatypes);
        } else {
            paramDatatypes = new String[0];
        }

        ArrayNode paramsNode = (ArrayNode) node.get("params");
        if (paramsNode != null && javaClasses != null) {
            params = new Object[paramsNode.size()];
            int i = 0;
            for (final JsonNode paramNode : paramsNode) {
                try {
                    params[i] = objectMapper.treeToValue(paramNode, javaClasses[i]);
                } catch (Exception e) {
                    logger.error("unable to deserialize to " + javaClasses[i] + " reason: " + e.getMessage());
                    params[i] = null;
                }
                i++;
            }
        } else {
            params = new Object[0];
        }

        return new Request(methodName, params, paramDatatypes, requestReplyId);

    }
}
