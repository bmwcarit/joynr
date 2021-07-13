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
package io.joynr.messaging;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Storage class for network configuration used to send messages.
 */

@Singleton
public class ConfigurableMessagingSettings implements MessagingSettings {
    public static final String PROPERTY_CC_CONNECTION_TYPE = "joynr.messaging.cc.connectiontype";

    public static final String PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID = "joynr.messaging.capabilitiesdirectoryparticipantid";
    public static final String PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID = "joynr.messaging.capabilitiesdirectorychannelid";
    public static final String PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN = "joynr.messaging.discoverydirectoriesdomain";
    public static final String PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS = "joynr.discovery.defaulttimeoutms";
    public static final String PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS = "joynr.discovery.defaultretryintervalms";
    public static final String PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS = "joynr.discovery.minimumretryintervalms";
    public static final String PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS = "joynr.discovery.provider.defaultexpirytimems";

    public static final String PROPERTY_SEND_MSG_RETRY_INTERVAL_MS = "joynr.messaging.sendmsgretryintervalms";
    public static final String PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE = "joynr.discovery.participantids_persistence_file";
    public static final String DEFAULT_PARTICIPANTIDS_PERSISTENCE_FILE = "joynr_participantIds.properties";

    public static final String PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS = "joynr.messaging.maximumparallelsends";

    public static final String PROPERTY_MAX_MESSAGE_SIZE = "joynr.messaging.maxmessagesize";

    public static final String PROPERTY_MESSAGING_MAXIMUM_TTL_MS = "joynr.messaging.maxttlms";
    public static final String PROPERTY_TTL_UPLIFT_MS = "joynr.messaging.ttlupliftms";
    public static final String PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS = "joynr.messaging.routingtablegraceperiodms";
    public static final String PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS = "joynr.messaging.routingtablecleanupintervalms";

    public static final String PROPERTY_ROUTING_MAX_RETRY_COUNT = "joynr.messaging.routingmaxretrycount";
    public static final long DEFAULT_ROUTING_MAX_RETRY_COUNT = -1;
    public static final String PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS = "joynr.messaging.maxDelayWithExponentialBackoffMs";
    public static final long DEFAULT_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF = -1;

    public static final String PROPERTY_GBIDS = "joynr.messaging.gbids";

    public static final String PROPERTY_GLOBAL_CAPABILITIES_DIRECTORY_URL = "joynr.messaging.gcd.url";

    private final long sendMsgRetryIntervalMs;
    private int maximumParallelSends;

    @Inject
    // CHECKSTYLE:OFF
    public ConfigurableMessagingSettings(@Named(PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                         @Named(PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maximumParallelSends) {
        // CHECKSTYLE:ON
        this.maximumParallelSends = maximumParallelSends;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
    }

    public int getMaximumParallelSends() {
        return maximumParallelSends;
    }

    @Override
    public long getSendMsgRetryIntervalMs() {
        return sendMsgRetryIntervalMs;
    }

    @Override
    public String toString() {
        return "MessagingSettings [sendMsgRetryIntervalMs=" + sendMsgRetryIntervalMs + "]";
    }
}
