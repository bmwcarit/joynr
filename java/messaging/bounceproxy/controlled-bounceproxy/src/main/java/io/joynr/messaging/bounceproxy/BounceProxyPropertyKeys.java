package io.joynr.messaging.bounceproxy;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Property keys for the configuration of a bounce proxy instance in a java
 * properties file.
 *
 * @author christina.strobel
 *
 */
public class BounceProxyPropertyKeys {

    public static final String PROPERTY_BOUNCE_PROXY_ID = "joynr.bounceproxy.id";
    public static final String PROPERTY_BOUNCE_PROXY_CONTROLLER_BASE_URL = "joynr.bounceproxy.controller.baseurl";
    public static final String PROPERTY_BOUNCEPROXY_URL_FOR_CC = "joynr.bounceproxy.url4cc";
    public static final String PROPERTY_BOUNCEPROXY_URL_FOR_BPC = "joynr.bounceproxy.url4bpc";

    private static final List<String> bounceProxySystemPropertyKeys = Arrays.asList(PROPERTY_BOUNCE_PROXY_ID, //
                                                                                    PROPERTY_BOUNCE_PROXY_CONTROLLER_BASE_URL, //
                                                                                    PROPERTY_BOUNCEPROXY_URL_FOR_CC, //
                                                                                    PROPERTY_BOUNCEPROXY_URL_FOR_BPC);

    public static final String PROPERTY_BOUNCE_PROXY_SEND_LIFECYCLE_REPORT_RETRY_INTERVAL_MS = "joynr.bounceproxy.send_lifecycle_report_retry_interval_ms";
    public static final String PROPERTY_BOUNCE_PROXY_MAX_SEND_SHUTDOWN_TIME_SECS = "joynr.bounceproxy.max_send_shutdown_report_time_secs";
    public static final String PROPERTY_BOUNCE_PROXY_MONITORING_FREQUENCY_MS = "joynr.bounceproxy.monitoring_frequency_ms";

    /**
     * Returns the keys for all properties that should be configured as system
     * properties. These typically are properties that don't have default values
     * and that should only be determined at runtime.
     *
     * @return the keys of all bounce proxy properties that should be configured
     * as system properties
     */
    public static List<String> getPropertyKeysForSystemProperties() {
        // make sure that the list is read-only
        return Collections.unmodifiableList(bounceProxySystemPropertyKeys);
    }

}
