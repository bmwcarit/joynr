package joynr;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.exceptions.JoynrException;

import java.io.IOException;

import com.fasterxml.jackson.annotation.JsonTypeInfo;
import com.fasterxml.jackson.annotation.JsonTypeInfo.As;
import com.fasterxml.jackson.annotation.JsonTypeInfo.Id;
import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeSerializer;
import com.fasterxml.jackson.databind.ser.std.StdSerializer;

public class JoynrApplicationException extends Exception implements JoynrException {

    private static final long serialVersionUID = 6620625652713563976L;

    @JsonTypeInfo(use = Id.CLASS, include = As.PROPERTY, property = "_typeName")
    @JsonSerialize(using = JoynrErrorEnumSerializer.class)
    @JsonDeserialize(using = JoynrErrorEnumDeSerializer.class)
    private Enum<?> error;

    /**
     * Constructor for deserializer
     */
    protected JoynrApplicationException() {
        super();
    }

    public JoynrApplicationException(Enum<?> error) {
        this.error = error;
    }

    public JoynrApplicationException(Enum<?> error, String message) {
        super(message);
        this.error = error;
    }

    public Enum<?> getError() {
        return this.error;
    }

    @Override
    public String toString() {
        return super.toString() + ": error: " + error;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        JoynrApplicationException other = (JoynrApplicationException) obj;
        if (getMessage() == null) {
            if (other.getMessage() != null) {
                return false;
            }
        } else if (!getMessage().equals(other.getMessage())) {
            return false;
        }
        if (error == null) {
            if (other.error != null) {
                return false;
            }
        } else if (!error.equals(other.error)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((getMessage() == null) ? 0 : getMessage().hashCode());
        result = prime * result + ((error == null) ? 0 : error.hashCode());
        return result;
    }

    @SuppressWarnings("rawtypes")
    public static class JoynrErrorEnumSerializer extends StdSerializer<Enum> {

        public JoynrErrorEnumSerializer() {
            super(Enum.class);
        }

        @Override
        public void serialize(Enum value, JsonGenerator jgen, SerializerProvider provider) throws IOException,
                                                                                          JsonGenerationException {
            provider.defaultSerializeValue(value, jgen);
        }

        @Override
        public void serializeWithType(Enum value,
                                      JsonGenerator jgen,
                                      SerializerProvider provider,
                                      TypeSerializer typeSer) throws IOException, JsonProcessingException {
            typeSer.writeTypePrefixForObject(value, jgen);
            jgen.writeFieldName(value.name());
            jgen.writeString(value.name());
            typeSer.writeTypeSuffixForObject(value, jgen);
        }
    }

    @SuppressWarnings("rawtypes")
    public static class JoynrErrorEnumDeSerializer extends StdDeserializer<Enum> {
        private static final long serialVersionUID = 1L;

        public JoynrErrorEnumDeSerializer() {
            super(Enum.class);
        }

        @Override
        public Enum deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException, JsonProcessingException {
            return null;
        }

        @Override
        public Enum deserializeWithType(JsonParser jp, DeserializationContext ctxt, TypeDeserializer typeDeserializer)
                                                                                                                      throws IOException,
                                                                                                                      JsonProcessingException {
            Enum result = (Enum) typeDeserializer.deserializeTypedFromObject(jp, ctxt);
            while (!(jp.getCurrentToken().equals(JsonToken.END_OBJECT))) {
                jp.nextToken();
            }
            return result;
        }

    }

}
