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

import org.apache.http.auth.AuthScope;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.client.CredentialsProvider;
import org.apache.http.impl.client.BasicCredentialsProvider;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.impl.client.HttpClients;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

/**
 * Factory class to build an AsyncHttpClient. If the MessagingSettings contain a proxyserver address the proxy is used
 * to create the http client.
 */

public class HttpClientProvider implements Provider<CloseableHttpClient> {
    protected final MessagingSettings settings;
    protected Properties properties;
    private HttpConstants httpConstants;

    @Inject
    public HttpClientProvider(MessagingSettings settings,
                              @Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties properties,
                              HttpConstants httpConstants) {
        this.settings = settings;
        this.properties = properties;
        this.httpConstants = httpConstants;
    }

    @Override
    public CloseableHttpClient get() {
        return createHttpClient(settings, properties);
    }

    public CloseableHttpClient createHttpClient(MessagingSettings mySettings, Properties withProperties) {

        // Create a HTTP client that uses the default pooling connection manager
        // and is configured by system properties
        HttpClientBuilder httpClientBuilder = HttpClients.custom()
                                                         .setDefaultCredentialsProvider(null)
                                                         .setMaxConnTotal(httpConstants.getHTTP_MAXIMUM_CONNECTIONS_TOTAL())
                                                         .setMaxConnPerRoute(httpConstants.getHTTP_MAXIMUM_CONNECTIONS_TO_HOST())
                                                         .useSystemProperties();

        String proxyHost = withProperties.getProperty("http.proxyhost");
        if (proxyHost != null) {
            // default proxy port is 8080
            int proxyPort = 8080;
            try {
                proxyPort = Integer.parseInt(withProperties.getProperty("http.proxyport"));
            } catch (Exception e) {
            }

            String proxyUser = withProperties.getProperty("http.proxyuser");
            if (proxyUser != null) {
                String proxyPassword = withProperties.getProperty("http.proxypassword");
                // default password is empty string
                proxyPassword = proxyPassword == null ? "" : proxyPassword;

                CredentialsProvider credsProvider = new BasicCredentialsProvider();
                credsProvider.setCredentials(new AuthScope(proxyHost, proxyPort),
                                             new UsernamePasswordCredentials(proxyUser, proxyPassword));
                httpClientBuilder.setDefaultCredentialsProvider(credsProvider);
            }
        }
        return httpClientBuilder.build();
    }
}
