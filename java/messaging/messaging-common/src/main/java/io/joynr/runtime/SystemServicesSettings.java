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
package io.joynr.runtime;

/**
 * Constants for system service configuration keys in properties and binding names in guice modules
 */
public class SystemServicesSettings {

    public static final String PROPERTY_DISPATCHER_ADDRESS = "joynr.messaging.dispatcheraddress";
    public static final String PROPERTY_CC_MESSAGING_ADDRESS = "joynr.messaging.ccmessagingaddress";

    public static final String PROPERTY_SYSTEM_SERVICES_DOMAIN = "joynr.messaging.systemservicesdomain";
    public static final String PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID = "joynr.messaging.discoveryproviderparticipantid";
    public static final String PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID = "joynr.messaging.routingproviderparticipantid";
    public static final String PROPERTY_CC_REMOVE_STALE_DELAY_MS = "joynr.messaging.removestaledelayms";

    public static final String LIBJOYNR_MESSAGING_ADDRESS = "libjoynr_messaging_address";

    public static final String PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS = "joynr.capabilities.freshnessupdateintervalms";
}
