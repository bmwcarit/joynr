package io.joynr.messaging.http.operation;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

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

/**
 * Constants used by the messaging http request classes.
 */
@Singleton
public class HttpConstants {

    @Inject
    @Named("joynr.http.header.x_atmosphere_tracking_id")
    private String HEADER_X_ATMOSPHERE_TRACKING_ID;
    @Inject
    @Named("joynr.http.x_cache_index")
    private String X_CACHE_INDEX;
    @Inject
    @Named("joynr.http.header_content_type")
    private String HEADER_CONTENT_TYPE;
    @Inject
    @Named("joynr.http.header_accept")
    private String HEADER_ACCEPT;
    @Inject
    @Named("joynr.http.header_location")
    private String HEADER_LOCATION;
    @Inject
    @Named("joynr.http.application_json")
    private String APPLICATION_JSON;
    @Inject
    @Named("joynr.http.http_request_timeout_ms")
    private int HTTP_REQUEST_TIMEOUT_MS;
    @Inject
    @Named("joynr.http.send_message_request_timeout_ms")
    private int SEND_MESSAGE_REQUEST_TIMEOUT;
    @Inject
    @Named("joynr.http.connection_timeout_ms")
    private int HTTP_CONNECTION_TIMEOUT_MS;
    @Inject
    @Named("joynr.http.maximum_connections_to_host")
    private int HTTP_MAXIMUM_CONNECTIONS_TO_HOST;
    @Inject
    @Named("joynr.http.maximum_connections_total")
    private int HTTP_MAXIMUM_CONNECTIONS_TOTAL;
    @Inject
    @Named("joynr.http.idle_connection_timeout_ms")
    private int HTTP_IDLE_CONNECTION_TIMEOUT_MS; // prevents IdleTimeoutThread from being started by AHC
    @Inject
    @Named("joynr.http.session_id_name")
    private String HTTP_SESSION_ID_NAME;

    // per connection

    public String getX_CACHE_INDEX() {
        return X_CACHE_INDEX;
    }

    public String getHEADER_CONTENT_TYPE() {
        return HEADER_CONTENT_TYPE;
    }

    public String getHEADER_ACCEPT() {
        return HEADER_ACCEPT;
    }

    public String getHEADER_LOCATION() {
        return HEADER_LOCATION;
    }

    public String getAPPLICATION_JSON() {
        return APPLICATION_JSON;
    }

    public int getHTTP_REQUEST_TIMEOUT_MS() {
        return HTTP_REQUEST_TIMEOUT_MS;
    }

    public int getSEND_MESSAGE_REQUEST_TIMEOUT() {
        return SEND_MESSAGE_REQUEST_TIMEOUT;
    }

    public int getHTTP_CONNECTION_TIMEOUT_MS() {
        return HTTP_CONNECTION_TIMEOUT_MS;
    }

    public int getHTTP_MAXIMUM_CONNECTIONS_TO_HOST() {
        return HTTP_MAXIMUM_CONNECTIONS_TO_HOST;
    }

    public int getHTTP_MAXIMUM_CONNECTIONS_TOTAL() {
        return HTTP_MAXIMUM_CONNECTIONS_TOTAL;
    }

    public int getHTTP_IDLE_CONNECTION_TIMEOUT_MS() {
        return HTTP_IDLE_CONNECTION_TIMEOUT_MS;
    }

    public String getHEADER_X_ATMOSPHERE_TRACKING_ID() {
        return HEADER_X_ATMOSPHERE_TRACKING_ID;
    }

    public String getHTTP_SESSION_ID_NAME() {
        return HTTP_SESSION_ID_NAME;
    }
}
