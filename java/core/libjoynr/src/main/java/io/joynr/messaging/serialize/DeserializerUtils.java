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

import org.slf4j.Logger;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.ArrayNode;

import io.joynr.util.ReflectionUtils;
import io.joynr.util.ObjectMapper;

class ParamsAndParamDatatypesHolder {
    public final String[] paramDatatypes;
    public final Object[] params;

    public ParamsAndParamDatatypesHolder(String[] paramDatatypes, Object[] params) {
        this.paramDatatypes = paramDatatypes;
        this.params = params;
    }
}

class DeserializerUtils {

    static ParamsAndParamDatatypesHolder deserializeParams(ObjectMapper objectMapper, JsonNode node, Logger logger) {
        Class<?>[] javaClasses = null;
        String[] paramDatatypes;
        Object[] params;

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
                    logger.error("Unable to deserialize to {}. Reason:", javaClasses[i], e);
                    params[i] = null;
                }
                i++;
            }
        } else {
            params = new Object[0];
        }
        return new ParamsAndParamDatatypesHolder(paramDatatypes, params);
    }
}
