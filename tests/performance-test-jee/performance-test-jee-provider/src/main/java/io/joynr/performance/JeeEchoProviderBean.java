/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.performance;

import jakarta.ejb.Stateless;
import com.google.inject.Inject;

import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.tests.performance.EchoSubscriptionPublisher;
import joynr.tests.performance.EchoSync;
import joynr.tests.performance.Types.ComplexStruct;

@Stateless
@ServiceProvider(serviceInterface = EchoSync.class)
@ProviderDomain("performance_test_domain")
public class JeeEchoProviderBean implements EchoSync {

    @Inject
    public JeeEchoProviderBean(@SubscriptionPublisher EchoSubscriptionPublisher echoSubscriptionPublisher) {
    }

    @Override
    public String echoString(String data) {
        return data;
    }

    @Override
    public Byte[] echoByteArray(Byte[] data) {
        return data;
    }

    @Override
    public ComplexStruct echoComplexStruct(ComplexStruct data) {
        return data;
    }

    @Override
    public String getSimpleAttribute() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void setSimpleAttribute(String simpleAttribute) {
        // TODO Auto-generated method stub

    }
}
