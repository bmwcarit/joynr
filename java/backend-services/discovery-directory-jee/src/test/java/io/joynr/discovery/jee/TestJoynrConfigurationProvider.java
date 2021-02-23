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
package io.joynr.discovery.jee;

import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.GCD_GBID;
import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.VALID_GBIDS;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;
import javax.inject.Named;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;

@Singleton
public class TestJoynrConfigurationProvider {

    static final String VALID_GBIDS_ARRAY = "joynrdefaultgbid,testGbid2,testGbid3";
    static final String JOYNR_DEFAULT_GCD_GBID = "joynrdefaultgbid";
    static final long DEFAULT_EXPIRY_INTERVAL_MS = 60000;

    @Produces
    @JoynrProperties
    public Properties getJoynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");
        joynrProperties.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                    "test-discovery-directory-joynr.properties");
        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String getJoynrLocalDomain() {
        return "io.joynrtest";
    }

    @Produces
    @Named(GCD_GBID)
    public String getGcdGbid() {
        return JOYNR_DEFAULT_GCD_GBID;
    }

    @Produces
    @Named(VALID_GBIDS)
    public String getValidGbids() {
        return VALID_GBIDS_ARRAY;
    }

    @Produces
    @Named(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS)
    public long getDefaultExpiryTimeMs() {
        return DEFAULT_EXPIRY_INTERVAL_MS;
    }
}
