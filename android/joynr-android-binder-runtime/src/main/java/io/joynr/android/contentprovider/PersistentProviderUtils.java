/*-
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
package io.joynr.android.contentprovider;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;
import static io.joynr.messaging.MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE;
import static io.joynr.util.VersionUtil.getVersionFromAnnotation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Properties;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.runtime.PropertyLoader;
import joynr.types.DiscoveryEntry;

public class PersistentProviderUtils {

    private static final Logger logger = LoggerFactory.getLogger(PersistentProviderUtils.class);

    /**
     * Gathers information from [PersistentProvider] and builds a [DiscoveryEntry] for that provider
     *
     * @param provider PersistentProvider object
     * @return Discovery Entry
     */
    protected static DiscoveryEntry generatePrimitiveDiscoveryEntry(
            PersistentJoynrContentProvider.PersistentProvider provider
    ) {

        DiscoveryEntry discoveryEntry = null;

        if (provider != null && provider.getDomain() != null && provider.getProviderQos() != null && provider.getJoynrProvider() != null) {

            ProviderContainerFactory providerContainerFactory = AndroidBinderRuntime.getInjector().getInstance(ProviderContainerFactory.class);
            ProviderContainer providerContainer = providerContainerFactory.create(provider.getJoynrProvider());
            PropertiesFileParticipantIdStorage participantIdStorage =
                    AndroidBinderRuntime
                            .getInjector()
                            .getInstance(PropertiesFileParticipantIdStorage.class);

            String participantId = participantIdStorage.getProviderParticipantId(
                    provider.getDomain(),
                    providerContainer.getInterfaceName(),
                    providerContainer.getMajorVersion());

            String defaultPublicKeyId = "";

            discoveryEntry = new DiscoveryEntry(
                    getVersionFromAnnotation(provider.getJoynrProvider().getClass()),
                    provider.getDomain(),
                    providerContainer.getInterfaceName(),
                    participantId,
                    provider.getProviderQos(),
                    System.currentTimeMillis(),
                    System.currentTimeMillis() + getDefaultExpiryTimeMs(),
                    defaultPublicKeyId);

            logger.info("Generated DiscoveryEntry " + discoveryEntry);
        } else {
            logger.error("Unable to generate DiscoveryEntry. PersistentProvider "
                    + provider + " has null variables.");
        }


        return discoveryEntry;
    }

    /**
     * Gets the Default Expiry Time in Milliseconds
     *
     * @return the Default Expiry Time in Milliseconds
     */
    protected static long getDefaultExpiryTimeMs() {
        Properties defaultMessagingProperties = PropertyLoader.loadProperties(DEFAULT_MESSAGING_PROPERTIES_FILE);
        return Long.parseLong(defaultMessagingProperties.getProperty(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS));
    }

}
