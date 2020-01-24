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
package io.joynr.arbitration;

import java.util.Collection;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;

/**
 * Provide an implementation of this class as part of the
 * {@link DiscoveryQos} (using one of the constructors which take this as
 * an argument) in order to have control over which discovery entries are
 * used as result of the arbitration.
 *
 * @see DiscoveryEntry
 * @see DiscoveryQos
 * @see ArbitrationStrategy
 */
public abstract class ArbitrationStrategyFunction {

    /**
     * Implement this method so that it selects all relevant, discovered
     * capabilities which should be used in the result of the arbitration.
     *
     * @param parameters the parameters which can be used during selection.
     * @param capabilities the list of candidate discovery entries from which
     * to select the relevant ones.
     * 
     * @return the collection of discovery entries which should be used in the
     * arbitration result. A value of <code>null</code> or an empty collection
     * are used to indicate that there was no match.
     */
    protected abstract Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                              Collection<DiscoveryEntryWithMetaInfo> capabilities);

    protected Optional<CustomParameter> findQosParameter(DiscoveryEntry discoveryEntry, String parameterName) {
        for (CustomParameter parameter : discoveryEntry.getQos().getCustomParameters()) {
            if (parameterName.equals(parameter.getName())) {
                return Optional.of(parameter);
            }
        }
        return Optional.empty();

    }
}
