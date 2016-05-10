package io.joynr.provider;

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

import joynr.types.ProviderQos;

public abstract class AbstractJoynrProvider implements JoynrProvider {
    /**
     * Provider quality of service settings
     * @deprecated Will be removed by end of the year 2016. Use external ProviderQos as input for
     * <code>io.joynr.runtime.JoynrRuntime#registerProvider(String, JoynrProvider, ProviderQos)}</code> instead.
     */
    @Deprecated
    protected ProviderQos providerQos = new ProviderQos();

    public AbstractJoynrProvider() {
    }

    /**
     * @deprecated Will be removed by end of the year 2016. Use external ProviderQos as input for
     * <code>io.joynr.runtime.JoynrRuntime#registerProvider(String, JoynrProvider, ProviderQos)}</code> instead.
     * @return provider QoS that applies to this provider instance.
     */
    @Deprecated
    public ProviderQos getProviderQos() {
        return providerQos;
    }

}
