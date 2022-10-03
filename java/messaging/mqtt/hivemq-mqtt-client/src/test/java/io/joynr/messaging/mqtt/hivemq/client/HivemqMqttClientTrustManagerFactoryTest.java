/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt.hivemq.client;

import java.security.KeyStore;
import javax.net.ssl.TrustManagerFactory;
import java.security.NoSuchAlgorithmException;
import java.security.KeyStoreException;
import org.junit.Test;
import org.mockito.Mock;
import static org.junit.Assert.assertNotNull;

public class HivemqMqttClientTrustManagerFactoryTest {

    @Mock
    KeyStore keystore;
    HivemqMqttClientTrustManagerFactory factory;

    @Test
    public void getTrustManagerFactoryTest() throws NoSuchAlgorithmException, KeyStoreException {
        factory = new HivemqMqttClientTrustManagerFactory();
        TrustManagerFactory trustManagerFactory = factory.getTrustManagerFactory(keystore);
        assertNotNull(trustManagerFactory);
    }
}
