package io.joynr.proxy;

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

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.dispatcher.rpc.JoynrInterface;

public class ProxyBuilderFactory {

    private LocalCapabilitiesDirectory capabilitiesDirectory;
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;

    public ProxyBuilderFactory(LocalCapabilitiesDirectory capabilitiesDirectory,
                               ProxyInvocationHandlerFactory proxyInvocationHandlerFactory) {
        this.capabilitiesDirectory = capabilitiesDirectory;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
    }

    public <T extends JoynrInterface> ProxyBuilder<T> get(String domain, Class<T> interfaceClass) {
        return new ProxyBuilderDefaultImpl<T>(capabilitiesDirectory,
                                              domain,
                                              interfaceClass,
                                              proxyInvocationHandlerFactory);
    }
}
