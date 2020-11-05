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
package joynr.system.RoutingTypes;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.google.inject.Inject;

import io.joynr.exceptions.JoynrSerializationException;
import io.joynr.util.ObjectMapper;

public class RoutingTypesUtil {

    @Inject
    private static ObjectMapper objectMapper;

    public static String toAddressString(Address address) {
        try {
            return objectMapper.writeValueAsString(address);
        } catch (JsonProcessingException e) {
            throw new JoynrSerializationException("unable to serialize address: " + address + " reason:"
                    + e.getMessage());
        }
    }

    public static Address fromAddressString(String addressString) {
        try {
            return objectMapper.readValue(addressString, Address.class);
        } catch (IOException e) {
            throw new JoynrSerializationException("unable to deserialize address: " + addressString + " reason:"
                    + e.getMessage());
        }
    }
}
