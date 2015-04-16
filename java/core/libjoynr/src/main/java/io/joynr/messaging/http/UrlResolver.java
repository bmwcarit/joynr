package io.joynr.messaging.http;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.http.operation.HttpConstants;
import io.joynr.messaging.util.Utilities;
import io.joynr.runtime.PropertyLoader;

import java.net.URL;
import java.util.List;
import java.util.Properties;

import javax.annotation.Nullable;
import javax.inject.Inject;

import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.name.Named;

public class UrlResolver {

    private static final Logger logger = LoggerFactory.getLogger(UrlResolver.class);

    private final HttpConstants httpConstants;
    private final Properties hosts;
    private final LocalChannelUrlDirectoryClient channelUrlClient;

    @Inject
    public UrlResolver(HttpConstants httpConstants,
                       @Named(ConfigurableMessagingSettings.PROPERTY_HOSTS_FILENAME) String hostsFileName,
                       LocalChannelUrlDirectoryClient channelUrlClient) {
        this.httpConstants = httpConstants;
        this.channelUrlClient = channelUrlClient;

        hosts = PropertyLoader.loadProperties(hostsFileName);
    }

    String mapHost(String url) throws Exception {
        URL originalUrl = new URL(url);
        String host = originalUrl.getHost();

        if (hosts.containsKey(host)) {

            String[] mappedHostInfo = hosts.getProperty(host).split(":");
            String mappedHost = mappedHostInfo[0];

            int port = originalUrl.getPort();
            if (mappedHostInfo.length >= 2) {
                port = Integer.parseInt(mappedHostInfo[1]);
            }

            String path = originalUrl.getFile();
            if (mappedHostInfo.length >= 3) {
                String mappedPathFind = mappedHostInfo[2];

                String pathReplace = "";
                if (mappedHostInfo.length >= 4) {
                    pathReplace = mappedHostInfo[3];
                }
                path = path.replaceFirst(mappedPathFind, pathReplace);
            }

            URL newURL = new URL(originalUrl.getProtocol(), mappedHost, port, path);
            url = newURL.toExternalForm();
        }

        return url;
    }

    private String encodeSendUrl(String encodedChannelUrl) {

        if (Utilities.isSessionEncodedInUrl(encodedChannelUrl, httpConstants.getHTTP_SESSION_ID_NAME())) {
            String channelUrlWithoutSessionId = Utilities.getUrlWithoutSessionId(encodedChannelUrl,
                                                                                 httpConstants.getHTTP_SESSION_ID_NAME());
            String sessionId = Utilities.getSessionId(encodedChannelUrl, httpConstants.getHTTP_SESSION_ID_NAME());

            return Utilities.getSessionEncodedUrl(channelUrlWithoutSessionId + "message/",
                                                  httpConstants.getHTTP_SESSION_ID_NAME(),
                                                  sessionId);

        } else {
            return encodedChannelUrl + "message/";
        }
    }

    @Nullable
    public String getSendUrl(String channelId) {

        ChannelUrlInformation channelUrlInfo = channelUrlClient.getUrlsForChannel(channelId);
        String url = null;

        List<String> urls = channelUrlInfo.getUrls();
        if (!urls.isEmpty()) {
            // in case sessions are used and the session is encoded in the URL,
            // we need to strip that from the URL and append session ID at the end
            String encodedChannelUrl = urls.get(0); // TODO handle trying multiple channelUrls
            url = encodeSendUrl(encodedChannelUrl);

            try {
                url = mapHost(url);
            } catch (Exception e) {
                logger.error("error in URL mapping while sending to channnelId: {} reason: {}",
                             channelId,
                             e.getMessage());
            }

        }

        return url;
    }
}
