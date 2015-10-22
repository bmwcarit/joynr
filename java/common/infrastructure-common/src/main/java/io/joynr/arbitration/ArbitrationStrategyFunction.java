package io.joynr.arbitration;

import java.util.Collection;
import java.util.Map;

import javax.annotation.CheckForNull;

import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;

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

public abstract class ArbitrationStrategyFunction {

    abstract DiscoveryEntry select(Map<String, String> parameters, Collection<DiscoveryEntry> capabilities);

    @CheckForNull
    protected CustomParameter findQosParameter(DiscoveryEntry discoveryEntry, String parameterName) {
        for (CustomParameter parameter : discoveryEntry.getQos().getCustomParameters()) {
            if (parameterName.equals(parameter.getName())) {
                return parameter;
            }
        }
        return null;

    }
}
