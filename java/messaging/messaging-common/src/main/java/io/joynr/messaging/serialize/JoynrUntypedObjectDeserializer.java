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

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.deser.std.UntypedObjectDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeDeserializer;

public class JoynrUntypedObjectDeserializer extends UntypedObjectDeserializer {
    private static final long serialVersionUID = 1L;
    transient private TypeDeserializer typeDeserializer;

    public JoynrUntypedObjectDeserializer(TypeDeserializer typeDeserializer) {
        super();
        this.typeDeserializer = typeDeserializer;
    }

    @Override
    /**
     * Arrays are not serialized with type information, so also need to deserialize them without
     */
    public Object deserializeWithType(JsonParser jp, DeserializationContext ctxt, TypeDeserializer withTypeDeserializer)
                                                                                                                        throws IOException,
                                                                                                                        JsonProcessingException {
        JsonToken t = jp.getCurrentToken();
        if (t == JsonToken.START_ARRAY) {
            return super.deserialize(jp, ctxt);
        }

        return super.deserializeWithType(jp, ctxt, withTypeDeserializer);
    }

    @Override
    /**
     * All joynr objects are serialized with type information, so deserialize them as such
     */
    public Object deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException, JsonProcessingException {
        JsonToken t = jp.getCurrentToken();
        if (t == JsonToken.START_OBJECT) {
            return super.deserializeWithType(jp, ctxt, typeDeserializer);
        }

        return super.deserialize(jp, ctxt);
    }
}
