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
package io.joynr.messaging.http;

import java.util.Optional;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;

public class ServletHttpGlobalAddressFactory extends HttpGlobalAddressFactory {

    private static final String SUPPORTED_TRANSPORT_SERVLET = "servlet";
    private String hostPath;
    private String contextRoot;
    private String myChannelId;

    @Inject
    public ServletHttpGlobalAddressFactory(@Named(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH) String hostPath,
                                           @Named(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT) String contextRoot,
                                           @Named(MessagingPropertyKeys.CHANNELID) String myChannelId) {

        if (hostPath.endsWith("/")) {
            hostPath = hostPath.substring(0, hostPath.length() - 1);
        }

        this.hostPath = hostPath;
        this.contextRoot = contextRoot;
        this.myChannelId = myChannelId;
    }

    @Override
    protected String getMyChannelId() {
        return myChannelId;
    }

    @Override
    protected String getMessagingEndpointUrl() {
        return hostPath + contextRoot + "/channels/" + myChannelId + "/";
    }

    @Override
    public boolean supportsTransport(Optional<String> transport) {
        return SUPPORTED_TRANSPORT_SERVLET.equalsIgnoreCase(transport.get());
    }
}
