package io.joynr.arbitration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.capabilities.CapabilityEntry;

import java.util.Collection;
import java.util.Map;

import joynr.types.CustomParameter;

public class KeywordArbitrationStrategyFunction extends ArbitrationStrategyFunction {

    @Override
    public CapabilityEntry select(Map<String, String> parameters, Collection<CapabilityEntry> capabilities) {
        String requestedKeyword = parameters.get(ArbitrationConstants.KEYWORD_PARAMETER);
        CapabilityEntry capabilityWithKeyword = null;

        for (CapabilityEntry capEntry : capabilities) {
            // Search for a matching keyword parameter
            CustomParameter keywordParameter = findQosParameter(capEntry, ArbitrationConstants.KEYWORD_PARAMETER);
            if (keywordParameter != null) {
                String currentKeyword = keywordParameter.getValue();
                if (currentKeyword.equals(requestedKeyword)) {
                    capabilityWithKeyword = capEntry;
                    break;
                }
            }
        }

        return capabilityWithKeyword;
    }
}
