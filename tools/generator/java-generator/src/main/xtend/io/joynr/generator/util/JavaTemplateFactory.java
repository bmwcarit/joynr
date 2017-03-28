package io.joynr.generator.util;

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

import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FMapType;

import io.joynr.generator.communicationmodel.ComplexTypeTemplate;
import io.joynr.generator.communicationmodel.EnumTypeTemplate;
import io.joynr.generator.communicationmodel.MapTypeTemplate;
import io.joynr.generator.interfaces.InterfaceAsyncTemplate;
import io.joynr.generator.interfaces.InterfaceBroadcastTemplate;
import io.joynr.generator.interfaces.InterfaceFireAndForgetTemplate;
import io.joynr.generator.interfaces.InterfaceSubscriptionTemplate;
import io.joynr.generator.interfaces.InterfaceSyncTemplate;
import io.joynr.generator.interfaces.InterfacesTemplate;
import io.joynr.generator.provider.DefaultInterfaceProviderTemplate;
import io.joynr.generator.provider.InterfaceAbstractProviderTemplate;
import io.joynr.generator.provider.InterfaceProviderTemplate;
import io.joynr.generator.provider.InterfaceSubscriptionPublisherImplTemplate;
import io.joynr.generator.provider.InterfaceSubscriptionPublisherTemplate;
import io.joynr.generator.proxy.InterfaceProxyTemplate;

public interface JavaTemplateFactory {
    InterfacesTemplate createInterfacesTemplate(FInterface francaIntf);

    InterfaceSyncTemplate createInterfaceSyncTemplate(FInterface francaIntf);

    InterfaceAsyncTemplate createInterfaceAsyncTemplate(FInterface francaIntf);

    InterfaceFireAndForgetTemplate createInterfaceFireAndForgetTemplate(FInterface francaIntf);

    InterfaceSubscriptionTemplate createInterfaceSubscriptionTemplate(FInterface francaIntf);

    InterfaceBroadcastTemplate createInterfaceBroadcastTemplate(FInterface francaIntf);

    InterfaceProviderTemplate createInterfaceProviderTemplate(FInterface francaIntf);

    DefaultInterfaceProviderTemplate createDefaultInterfaceProviderTemplate(FInterface francaIntf);

    InterfaceAbstractProviderTemplate createInterfaceAbstractProviderTemplate(FInterface francaIntf);

    InterfaceSubscriptionPublisherTemplate createSubscriptionPublisherTemplate(FInterface francaIntf);

    InterfaceSubscriptionPublisherImplTemplate createSubscriptionPublisherImplTemplate(FInterface francaIntf);

    InterfaceProxyTemplate createInterfaceProxyTemplate(FInterface francaIntf);

    ComplexTypeTemplate createComplexTypeTemplate(FCompoundType type);

    EnumTypeTemplate createEnumTypeTemplate(FEnumerationType type);

    MapTypeTemplate createMapTypeTemplate(FMapType type);
}
