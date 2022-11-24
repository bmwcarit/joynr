/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.capabilities.helpers;

import static org.junit.Assert.assertNotNull;

import org.mockito.ArgumentMatcher;

import joynr.types.GlobalDiscoveryEntry;

public class GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher implements ArgumentMatcher<GlobalDiscoveryEntry> {

    private final GlobalDiscoveryEntry expected;

    public GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(final GlobalDiscoveryEntry expected) {
        this.expected = expected;
    }

    @Override
    public boolean matches(final GlobalDiscoveryEntry argument) {
        assertNotNull(argument);
        return DiscoveryEntryMatchHelper.globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(expected, argument);
    }
}