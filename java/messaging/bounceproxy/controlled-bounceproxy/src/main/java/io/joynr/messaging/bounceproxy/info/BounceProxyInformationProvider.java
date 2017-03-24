package io.joynr.messaging.bounceproxy.info;

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

import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;
import io.joynr.messaging.info.BounceProxyInformation;

import java.net.URI;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

/**
 * Provider to construct bounce proxy information from properties.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyInformationProvider implements Provider<BounceProxyInformation> {

    private final BounceProxyInformation bpInfo;

    @Inject
    public BounceProxyInformationProvider(@Named(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_ID) String bpId,
                                          @Named(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_CC) String urlForCc) {
        bpInfo = new BounceProxyInformation(bpId, URI.create(urlForCc));
    }

    @Override
    public BounceProxyInformation get() {
        return bpInfo;
    }

}
