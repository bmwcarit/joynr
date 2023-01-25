/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt.settings;

public final class LimitAndBackpressureSettings {

    private LimitAndBackpressureSettings() {
    }

    public static final String PROPERTY_BACKPRESSURE_ENABLED = "joynr.messaging.backpressure.enabled";
    public static final String PROPERTY_MAX_INCOMING_MQTT_REQUESTS = "joynr.messaging.maxincomingmqttrequests";
    public static final String PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD = "joynr.messaging.backpressure.incomingmqttrequests.lowerthreshold";
}
