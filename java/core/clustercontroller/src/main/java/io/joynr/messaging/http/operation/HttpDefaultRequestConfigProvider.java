package io.joynr.messaging.http.operation;

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

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingSettings;

import java.util.Properties;

import org.apache.http.HttpHost;
import org.apache.http.client.config.RequestConfig;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

/**
 * Factory class to build a RequestConfig
 */

public class HttpDefaultRequestConfigProvider implements Provider<RequestConfig> {
    private final RequestConfig defaultRequestConfig;

    @Inject
    public HttpDefaultRequestConfigProvider(MessagingSettings settings,
                                            @Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties properties,
                                            HttpConstants httpConstants) {

        RequestConfig.Builder configBuilder = RequestConfig.custom()
                                                           .setStaleConnectionCheckEnabled(true)
                                                           .setConnectionRequestTimeout(httpConstants.getHTTP_REQUEST_TIMEOUT_MS())
                                                           .setConnectTimeout(httpConstants.getHTTP_CONNECTION_TIMEOUT_MS());

        String proxyHost = properties.getProperty("http.proxyhost");
        if (proxyHost != null) {
            // default proxy port is 8080
            int proxyPort = 8080;
            try {
                proxyPort = Integer.parseInt(properties.getProperty("http.proxyport"));
            } catch (Exception e) {
            }
            HttpHost proxy = new HttpHost(proxyHost, proxyPort);
            configBuilder.setProxy(proxy);
        }

        defaultRequestConfig = configBuilder.build();
    }

    @Override
    public RequestConfig get() {
        return defaultRequestConfig;
    }

}
