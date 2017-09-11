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

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.fasterxml.jackson.databind.ser.std.NonTypedScalarSerializerBase;

public class JoynrEnumSerializer extends NonTypedScalarSerializerBase<Enum> {

    public JoynrEnumSerializer() {
        super(Enum.class);
    }

    @Override
    public void serialize(Enum value, JsonGenerator jgen, SerializerProvider provider) throws IOException,
                                                                                      JsonGenerationException {
        jgen.writeString(value.name());

    }

}
