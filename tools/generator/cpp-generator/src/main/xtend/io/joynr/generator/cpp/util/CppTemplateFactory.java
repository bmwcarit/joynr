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
package io.joynr.generator.cpp.util;

import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FMapType;

import io.joynr.generator.cpp.communicationmodel.EnumCppTemplate;
import io.joynr.generator.cpp.communicationmodel.EnumHTemplate;
import io.joynr.generator.cpp.communicationmodel.MapCppTemplate;
import io.joynr.generator.cpp.communicationmodel.MapHTemplate;
import io.joynr.generator.cpp.communicationmodel.TypeCppTemplate;
import io.joynr.generator.cpp.communicationmodel.TypeHTemplate;
import io.joynr.generator.cpp.defaultProvider.DefaultInterfaceProviderCppTemplate;
import io.joynr.generator.cpp.defaultProvider.DefaultInterfaceProviderHTemplate;
import io.joynr.generator.cpp.interfaces.InterfaceCppTemplate;
import io.joynr.generator.cpp.interfaces.InterfaceHTemplate;
import io.joynr.generator.cpp.joynrmessaging.InterfaceJoynrMessagingConnectorCppTemplate;
import io.joynr.generator.cpp.joynrmessaging.InterfaceJoynrMessagingConnectorHTemplate;
import io.joynr.generator.cpp.provider.InterfaceAbstractProviderCppTemplate;
import io.joynr.generator.cpp.provider.InterfaceAbstractProviderHTemplate;
import io.joynr.generator.cpp.provider.InterfaceProviderCppTemplate;
import io.joynr.generator.cpp.provider.InterfaceProviderHTemplate;
import io.joynr.generator.cpp.provider.InterfaceRequestCallerCppTemplate;
import io.joynr.generator.cpp.provider.InterfaceRequestCallerHTemplate;
import io.joynr.generator.cpp.provider.InterfaceRequestInterpreterCppTemplate;
import io.joynr.generator.cpp.provider.InterfaceRequestInterpreterHTemplate;
import io.joynr.generator.cpp.proxy.IInterfaceConnectorHTemplate;
import io.joynr.generator.cpp.proxy.InterfaceAsyncProxyCppTemplate;
import io.joynr.generator.cpp.proxy.InterfaceAsyncProxyHTemplate;
import io.joynr.generator.cpp.proxy.InterfaceFireAndForgetProxyCppTemplate;
import io.joynr.generator.cpp.proxy.InterfaceFireAndForgetProxyHTemplate;
import io.joynr.generator.cpp.proxy.InterfaceProxyBaseCppTemplate;
import io.joynr.generator.cpp.proxy.InterfaceProxyBaseHTemplate;
import io.joynr.generator.cpp.proxy.InterfaceProxyCppTemplate;
import io.joynr.generator.cpp.proxy.InterfaceProxyHTemplate;
import io.joynr.generator.cpp.proxy.InterfaceSyncProxyCppTemplate;
import io.joynr.generator.cpp.proxy.InterfaceSyncProxyHTemplate;

public interface CppTemplateFactory {
    InterfaceHTemplate createInterfaceHTemplate(FInterface francaIntf);

    InterfaceCppTemplate createInterfaceCppTemplate(FInterface francaIntf);

    DefaultInterfaceProviderHTemplate createDefaultInterfaceProviderHTemplate(FInterface francaIntf);

    DefaultInterfaceProviderCppTemplate createDefaultInterfaceProviderCppTemplate(FInterface francaIntf);

    InterfaceJoynrMessagingConnectorHTemplate createInterfaceJoynrMessagingConnectorHTemplate(FInterface francaIntf);

    InterfaceJoynrMessagingConnectorCppTemplate createInterfaceJoynrMessagingConnectorCppTemplate(FInterface francaIntf);

    InterfaceRequestInterpreterHTemplate createInterfaceRequestInterpreterHTemplate(FInterface francaIntf);

    InterfaceRequestInterpreterCppTemplate createInterfaceRequestInterpreterCppTemplate(FInterface francaIntf);

    InterfaceRequestCallerHTemplate createInterfaceRequestCallerHTemplate(FInterface francaIntf);

    InterfaceRequestCallerCppTemplate createInterfaceRequestCallerCppTemplate(FInterface francaIntf);

    InterfaceProviderCppTemplate createInterfaceProviderCppTemplate(FInterface francaIntf);

    InterfaceProviderHTemplate createInterfaceProviderHTemplate(FInterface francaIntf);

    InterfaceAbstractProviderCppTemplate createInterfaceAbstractProviderCppTemplate(FInterface francaIntf);

    InterfaceAbstractProviderHTemplate createInterfaceAbstractProviderHTemplate(FInterface francaIntf);

    IInterfaceConnectorHTemplate createIInterfaceConnectorHTemplate(FInterface francaIntf);

    InterfaceProxyBaseHTemplate createInterfaceProxyBaseHTemplate(FInterface francaIntf);

    InterfaceProxyBaseCppTemplate createInterfaceProxyBaseCppTemplate(FInterface francaIntf);

    InterfaceProxyHTemplate createInterfaceProxyHTemplate(FInterface francaIntf);

    InterfaceProxyCppTemplate createInterfaceProxyCppTemplate(FInterface francaIntf);

    InterfaceSyncProxyHTemplate createInterfaceSyncProxyHTemplate(FInterface francaIntf);

    InterfaceSyncProxyCppTemplate createInterfaceSyncProxyCppTemplate(FInterface francaIntf);

    InterfaceAsyncProxyHTemplate createInterfaceAsyncProxyHTemplate(FInterface francaIntf);

    InterfaceAsyncProxyCppTemplate createInterfaceAsyncProxyCppTemplate(FInterface francaIntf);

    InterfaceFireAndForgetProxyHTemplate createInterfaceFireAndForgetProxyHTemplate(FInterface francaIntf);

    InterfaceFireAndForgetProxyCppTemplate createInterfaceFireAndForgetProxyCppTemplate(FInterface francaIntf);

    TypeHTemplate createTypeHTemplate(FCompoundType type);

    TypeCppTemplate createTypeCppTemplate(FCompoundType type);

    EnumHTemplate createEnumHTemplate(FEnumerationType type);

    EnumCppTemplate createEnumCppTemplate(FEnumerationType type);

    MapHTemplate createMapHTemplate(FMapType type);

    MapCppTemplate createMapCppTemplate(FMapType type);
}
