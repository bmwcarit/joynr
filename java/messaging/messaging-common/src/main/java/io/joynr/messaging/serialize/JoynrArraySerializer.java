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
import java.util.Map;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.ser.std.StdSerializer;

public class JoynrArraySerializer extends StdSerializer<Object[]> {
    private static final long serialVersionUID = 1L;
    Map<Class<?>, JsonSerializer<Object>> serializers = new HashMap<Class<?>, JsonSerializer<Object>>();

    public JoynrArraySerializer() {
        super(Object[].class, false);
    }

    @Override
    public void serialize(Object[] value, JsonGenerator gen, SerializerProvider provider) throws IOException {
        gen.writeStartArray();
        for (Object elem : value) {
            if (elem == null) {
                provider.defaultSerializeNull(gen);
            } else {
                Class<?> clazz = elem.getClass();
                JsonSerializer<Object> serializer = serializers.get(clazz);
                if (serializer == null) {
                    serializer = provider.findTypedValueSerializer(clazz, false, null);
                }
                serializer.serialize(elem, gen, provider);
            }
        }
        gen.writeEndArray();

    }

}
