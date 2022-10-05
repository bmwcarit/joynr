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
package io.joynr.messaging.mqtt;

import java.util.Properties;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.SystemServicesSettings;
import joynr.types.GlobalDiscoveryEntry;

@Singleton
public class MqttPropertiesFileParticipantIdStorage extends PropertiesFileParticipantIdStorage {

    private boolean sharedSubscriptionsEnabled;

    @Inject
    public MqttPropertiesFileParticipantIdStorage(@Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties joynrProperties,
                                                  @Named(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE) String persistenceFileName,
                                                  @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                                  @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String routingProviderParticipantId,
                                                  @Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry,
                                                  @Named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) boolean sharedSubscriptionsEnabled) {
        super(joynrProperties,
              persistenceFileName,
              discoveryProviderParticipantId,
              routingProviderParticipantId,
              capabilitiesDirectoryEntry);
        this.sharedSubscriptionsEnabled = sharedSubscriptionsEnabled;
    }

    @Override
    protected String createVariableParticipantId(String token) {
        if (sharedSubscriptionsEnabled) {
            throw new JoynrRuntimeException("Failed to retrieve fixed participantId for shared subscription provider with token "
                    + token);
        }
        return super.createVariableParticipantId(token);
    }

}
