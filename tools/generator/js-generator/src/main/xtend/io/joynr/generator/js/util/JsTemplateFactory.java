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
package io.joynr.generator.js.util;

import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FMapType;

import io.joynr.generator.js.communicationmodel.CompoundTypeGenerator;
import io.joynr.generator.js.communicationmodel.EnumTypeGenerator;
import io.joynr.generator.js.communicationmodel.MapTypeGenerator;
import io.joynr.generator.js.provider.ProviderGenerator;
import io.joynr.generator.js.provider.ProviderImplCreatorGenerator;
import io.joynr.generator.js.proxy.ProxyGenerator;

public interface JsTemplateFactory {
    ProxyGenerator createProxyGenerator(FInterface francaIntf);

    ProviderGenerator createProviderGenerator(FInterface francaIntf);

    ProviderImplCreatorGenerator createProviderImplCreatorGenerator(FInterface francaIntf);

    MapTypeGenerator createMapTypeGenerator(FMapType type);

    CompoundTypeGenerator createCompoundTypeGenerator(FCompoundType type);

    EnumTypeGenerator createEnumTypeGenerator(FEnumerationType type);
}
