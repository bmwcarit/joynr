/**
 *
 */
package io.joynr.jeeintegration.httpbridge;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import org.apache.http.client.HttpClient;

import com.google.inject.AbstractModule;

import io.joynr.messaging.http.operation.HttpClientProvider;

/**
 * Module which sets up the bindings for the {@link HttpBridgeEndpointRegistryClient}.
 *
 * @author clive.jevons commissioned by MaibornWolff
 */
public class HttpBridgeEndpointRegistryClientModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(HttpBridgeRegistryClient.class).to(HttpBridgeEndpointRegistryClient.class);
        bind(HttpClient.class).toProvider(HttpClientProvider.class);
    }

}
