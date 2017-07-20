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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.jsontype.TypeSerializer;
import com.fasterxml.jackson.databind.ser.std.StdSerializer;

/**
 * This class is required only because IndexedListSerializer is currently final. Would be better to override.
 *
 */
public class JoynrListSerializer extends StdSerializer<List<?>> {
    Map<Class<?>, JsonSerializer<Object>> serializers = new HashMap<Class<?>, JsonSerializer<Object>>();

    public JoynrListSerializer() {
        super(List.class, false);
    }

    @Override
    public void serialize(List<?> value, JsonGenerator jgen, SerializerProvider provider) throws IOException,
                                                                                         JsonGenerationException {

        jgen.writeStartArray();
        for (Object elem : value) {
            if (elem == null) {
                provider.defaultSerializeNull(jgen);
            } else {
                Class<?> clazz = elem.getClass();
                JsonSerializer<Object> serializer = serializers.get(clazz);
                if (serializer == null) {
                    serializer = provider.findTypedValueSerializer(clazz, false, null);
                }
                serializer.serialize(elem, jgen, provider);
            }
        }
        jgen.writeEndArray();

    }

    @Override
    public void serializeWithType(List<?> value, JsonGenerator jgen, SerializerProvider provider, TypeSerializer typeSer)
                                                                                                                         throws IOException,
                                                                                                                         JsonProcessingException {
        serialize(value, jgen, provider);
    }

}
