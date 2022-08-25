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
package joynr.exceptions;

import java.io.IOException;

import com.fasterxml.jackson.annotation.JsonTypeInfo.As;
import com.fasterxml.jackson.annotation.JsonTypeInfo.Id;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
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

import io.joynr.exceptions.JoynrException;

/**
 * Joynr exception used to return error enums defined in the corresponding
 * Franca model file from provider to consumer.
 */
public class ApplicationException extends Exception implements JoynrException {

    private static final long serialVersionUID = 6620625652713563976L;

    private static final String JSON_FIELD_NAME_ERROR_ENUM = "name";
    private static final String JSON_FIELD_NAME_TYPE = "_typeName";

    @JsonTypeInfo(use = Id.CLASS, include = As.PROPERTY, property = "_typeName")
    @JsonSerialize(using = JoynrErrorEnumSerializer.class)
    @JsonDeserialize(using = JoynrErrorEnumDeSerializer.class)
    private Enum<?> error;

    /**
     * Constructor for deserializer
     */
    protected ApplicationException() {
        super();
    }

    /**
     * Constructor for an ApplicationException with an error enum.
     *
     * @param error error enum to be reported
     */
    public ApplicationException(Enum<?> error) {
        super("ErrorValue: " + error);
        this.error = error;
    }

    /**
     * Constructor for an ApplicationException with an error enum and detail message.
     *
     * @param error error enum to be reported
     * @param message description of the reported error
     */
    public ApplicationException(Enum<?> error, String message) {
        super(message + " ErrorValue: " + error);
        this.error = error;
    }

    /**
     * @param <T> the concrete error enum type
     * @return the reported error enum
     */
    @SuppressWarnings("unchecked")
    public <T extends Enum<?>> T getError() {
        try {
            return (T) error;
        } catch (ClassCastException e) {
            throw new MethodInvocationException("cannot cast enum " + error);
        }
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
        ApplicationException other = (ApplicationException) obj;
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
        private static final long serialVersionUID = 1L;

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
            typeSer.writeCustomTypePrefixForObject(value, jgen, value.getClass().getName().replace("$", "."));
            jgen.writeFieldName(JSON_FIELD_NAME_ERROR_ENUM);
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
        public Enum deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException,
                                                                            JsonProcessingException {
            return null;
        }

        @SuppressWarnings("unchecked")
        @Override
        public Enum deserializeWithType(JsonParser jp,
                                        DeserializationContext ctxt,
                                        TypeDeserializer typeDeserializer) throws IOException, JsonProcessingException {
            String typeName = null;
            String enumName = null;

            if (!jp.getCurrentToken().equals(JsonToken.START_OBJECT)) {
                throw new IOException("Invalid Json format: parser does not point to START_OBJECT!");
            }
            if (!jp.getCurrentName().equals("error")) {
                throw new IOException("Invalid Json format: parser does not point to error enum");
            }

            while (!(jp.getCurrentToken().equals(JsonToken.END_OBJECT))) {
                jp.nextToken();
                if (!(jp.getCurrentToken().equals(JsonToken.FIELD_NAME))) {
                    continue;
                }
                if (jp.getText().equals(JSON_FIELD_NAME_TYPE)) {
                    jp.nextToken();
                    typeName = jp.getText();
                } else if (jp.getText().equals(JSON_FIELD_NAME_ERROR_ENUM)) {
                    jp.nextToken();
                    enumName = jp.getText();
                }
            }

            if (typeName == null) {
                throw new IOException("Invalid Json format: \"" + JSON_FIELD_NAME_TYPE + "\" not found!");
            }
            if (enumName == null) {
                throw new IOException("Invalid Json format: enum \"" + JSON_FIELD_NAME_ERROR_ENUM + "\" not found!");
            }

            try {
                return Enum.valueOf(Class.forName(typeName).asSubclass(Enum.class), enumName);
            } catch (ClassNotFoundException e) {
                try {
                    int indexOfLastDot = typeName.lastIndexOf(".");
                    typeName = new StringBuilder(typeName).replace(indexOfLastDot, indexOfLastDot + 1, "$").toString();
                    return Enum.valueOf(Class.forName(typeName).asSubclass(Enum.class), enumName);
                } catch (ClassNotFoundException e2) {
                    throw new IOException(e2);
                }
            }
        }
    }
}
