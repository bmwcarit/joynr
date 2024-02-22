/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.exceptions.JoynrIllegalStateException;

public class MqttCiphersuiteListFactory {

    private static final Logger logger = LoggerFactory.getLogger(MqttCiphersuiteListFactory.class);

    private List<String> cipherList;

    @Inject
    public MqttCiphersuiteListFactory(@Named(MqttModule.PROPERTY_KEY_MQTT_CIPHERSUITES) String cipherListString) {
        if (cipherListString.isEmpty()) {
            cipherList = new ArrayList<String>();
            return;
        }
        List<String> tempCipherList = Arrays.stream(cipherListString.split(","))
                                            .map(a -> a.trim())
                                            .collect(Collectors.toList());
        if (tempCipherList.size() == 0) {
            final String msg = "Cipher list has no entries. This should not be possible!";
            logger.error(msg);
            throw new JoynrIllegalStateException(msg);
        } else {
            for (String cipher : tempCipherList) {
                if (cipher.isEmpty()) {
                    final String msg = "Cipher must not be empty";
                    logger.error(msg);
                    throw new JoynrIllegalStateException(msg);
                }
            }
        }
        cipherList = tempCipherList;
    }

    public List<String> create() {
        return (cipherList != null) ? new ArrayList<>(cipherList) : null;
    }

}
