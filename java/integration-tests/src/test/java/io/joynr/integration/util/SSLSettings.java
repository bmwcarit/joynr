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
package io.joynr.integration.util;

public class SSLSettings {
    private final String keyStorePath;
    private final String trustStorePath;
    private final String keyStorePassword;
    private final String trustStorePassword;

    public SSLSettings(String keyStorePath, String trustStorePath, String keyStorePassword, String trustStorePassword) {
        this.keyStorePath = keyStorePath;
        this.trustStorePath = trustStorePath;
        this.keyStorePassword = keyStorePassword;
        this.trustStorePassword = trustStorePassword;
    }

    public String getKeyStorePath() {
        return keyStorePath;
    }

    public String getTrustStorePath() {
        return trustStorePath;
    }

    public String getKeyStorePassword() {
        return keyStorePassword;
    }

    public String getTrustStorePassword() {
        return trustStorePassword;
    }

}
