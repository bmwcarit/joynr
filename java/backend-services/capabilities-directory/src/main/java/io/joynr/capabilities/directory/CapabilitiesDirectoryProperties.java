package io.joynr.capabilities.directory;

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

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class CapabilitiesDirectoryProperties {

    private final Properties config;
    public static final String CONFIG_FILE_NAME = "capabilitiesDirectory.properties";
    private Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryProperties.class);

    public CapabilitiesDirectoryProperties() {
        config = getConfigProperties();
    }

    private Properties getConfigProperties() {

        InputStream resourceStream;
        URL resource = this.getClass().getClassLoader().getResource(CONFIG_FILE_NAME);
        try {
            resourceStream = resource.openStream();
        } catch (IOException e) {
            logger.error("The configuration file could not be located" + resource.toString(), e);
            resourceStream = null;
        }

        Properties properties = new Properties();
        try {
            if (resourceStream != null) {
                properties.load(resourceStream);
            }
        } catch (IOException ex) {
            logger.error("Could not load the configuration file: {}", CONFIG_FILE_NAME, ex);
        }
        return properties;
    }

    @Inject
    public CapabilitiesDirectoryProperties(@Named("capabilitiesDirectoryConfig") Properties config) {
        if (config == null) {
            throw new IllegalArgumentException("The configuration properties cannot be null.");
        }
        this.config = config;
    }

    public String getMessagingBounceProxyUrl() {
        return config.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);
    }

    public String getMessagingCreateChannelRetryIntervalMs() {
        return config.getProperty(ConfigurableMessagingSettings.PROPERTY_CREATE_CHANNEL_RETRY_INTERVAL_MS);
    }

    public String getMessagingSendMsgRetryIntervalMs() {
        return config.getProperty(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS);
    }

    public String getMessagingSendProxyAddress() {
        return config.getProperty("joynr.messaging.sendProxyAddress");
    }

    public String getMessagingSendProxyPort() {
        return config.getProperty("joynr.messaging.sendProxyPort");
    }

    public String getCapabilitiesDirectoryChannelId() {
        return config.getProperty("joynr.capabilitiesDirectory.channelId");
    }

    public String getCapabilitiesDirectoryInterface() {
        return config.getProperty("joynr.capabilitiesDirectory.interface");
    }

    public String getCapabilitiesDirectoryDomain() {
        return config.getProperty("joynr.capabilitiesDirectory.domain");
    }

    public String getCapabilitiesClientRequestTimeout() {
        return config.getProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT);
    }

    public String getMessagingLongPollRetryIntervalMs() {
        return config.getProperty("joynr.messaging.LongPollRetryIntervalMs");
    }

    public String getCapabilitiesDumpFile() {
        return config.getProperty("joynr.capabilitiesDirectory.dumpFile");
    }

    public Properties getProperties() {
        return config;
    }
}
