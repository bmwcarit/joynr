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
package io.joynr.test;

import java.util.Set;

import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ProviderRegistrar;
import joynr.types.ProviderQos;

public class TestRuntime implements JoynrRuntime {
    @Override
    @Deprecated
    public Future<Void> registerProvider(String domain, Object provider, ProviderQos providerQos) {
        return null;
    }

    @Override
    @Deprecated
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         boolean awaitGlobalRegistration) {
        return null;
    }

    @Override
    @Deprecated
    public Future<Void> registerProvider(String domain,
                                         Object provider,
                                         ProviderQos providerQos,
                                         String[] gbids,
                                         boolean awaitGlobalRegistration) {
        return null;
    }

    @Override
    public void unregisterProvider(String domain, Object provider) {

    }

    @Override
    public void registerStatelessAsyncCallback(StatelessAsyncCallback statelessAsyncCallback) {

    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(String domain, Class<T> interfaceClass) {
        return null;
    }

    @Override
    public void prepareForShutdown() {
    }

    @Override
    public void shutdown(boolean clear) {
    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(Set<String> domains, Class<T> interfaceClass) {
        return null;
    }

    @Override
    public ProviderRegistrar getProviderRegistrar(String domain, JoynrProvider provider) {
        return null;
    }

    @Override
    public GuidedProxyBuilder getGuidedProxyBuilder(Set<String> domains, Class<?> interfaceClass) {
        return null;
    }

}
