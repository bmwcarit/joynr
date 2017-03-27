package io.joynr.arbitration;

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

/**
 * DiscoveryScope indicates whther only locally registered capabilities are to be used in discovery, or if the global registry is also to be used.
 *
 */
public enum DiscoveryScope {
    /**
     * Only capabilities registered locally will be discovered
     */
    LOCAL_ONLY,
    /**
     * local capabilities will be discovered if available; otherwise the global registry will be queried
     */
    LOCAL_THEN_GLOBAL,
    /**
     * local capabilities and capabilities from the global registry will be combined and returned
     */
    LOCAL_AND_GLOBAL,
    /**
     * Only the global registry will be queried during discovery
     */
    GLOBAL_ONLY
};