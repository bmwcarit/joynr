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
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.ser.std.NonTypedScalarSerializerBase;
import com.fasterxml.jackson.databind.ser.std.NumberSerializers;
import com.fasterxml.jackson.databind.ser.std.NumberSerializers.IntLikeSerializer;

public class NumberSerializer extends NonTypedScalarSerializerBase<Number> {
    Map<Class<?>, JsonSerializer<?>> serializers = new HashMap<Class<?>, JsonSerializer<?>>();

    public NumberSerializer() {
        super(Number.class);
        IntLikeSerializer intLikeSerializer = new NumberSerializers.IntLikeSerializer();
        serializers.put(Byte.class, intLikeSerializer);
        serializers.put(Short.class, intLikeSerializer);
        serializers.put(Integer.class, new NumberSerializers.IntegerSerializer());
        serializers.put(Long.class, new NumberSerializers.LongSerializer());
        serializers.put(Float.class, new NumberSerializers.FloatSerializer());
        serializers.put(Double.class, new NumberSerializers.DoubleSerializer());
    }

    @SuppressWarnings("unchecked")
    @Override
    public void serialize(Number value, JsonGenerator jgen, SerializerProvider provider) throws IOException,
                                                                                        JsonProcessingException {
        JsonSerializer serializer = serializers.get(value.getClass());
        if (serializer == null) {
            throw new IllegalArgumentException("serialze called on type that is not supported: " + value.getClass());
        }
        serializer.serialize(value, jgen, provider);
    }

}
