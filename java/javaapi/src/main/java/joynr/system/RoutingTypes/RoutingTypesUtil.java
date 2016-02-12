package joynr.system.RoutingTypes;

import java.io.IOException;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.exceptions.JoynrIllegalStateException;

public class RoutingTypesUtil {

    static ObjectMapper objectMapper = new ObjectMapper();

    public static String toAddressString(Address address) {
        if (address instanceof ChannelAddress) {
            return ((ChannelAddress) address).getChannelId();
        } else if (address instanceof MqttAddress) {
            MqttAddress mqttAddress = (MqttAddress) address;
            try {
                return objectMapper.writeValueAsString(mqttAddress);
            } catch (JsonProcessingException e) {
                throw new JoynrIllegalStateException("MQTT address could not be serilaized: " + e.getMessage());
            }
        }
        throw new JoynrIllegalStateException("unable to convert address to string: unknown address type: " + address);
    }

    public static Address fromAddressString(String addressString) {
        //TODO channelId for ChannelAddress not yet serialized as object. Later this If will not be necessary
        if (addressString.startsWith("{")) {
            try {
                return objectMapper.readValue(addressString, MqttAddress.class);
            } catch (IOException e) {
                throw new JoynrIllegalStateException("unable to deserialize address: " + addressString + " reason:"
                        + e.getMessage());
            }
        } else {
            return new ChannelAddress(addressString);
        }

    }
}
