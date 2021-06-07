/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.dispatching;

public interface DirectoryListener<T> {

    /**
     * Callback: Once a listener has added itself to the Directory, the Directory will call this method whenever
     * a new entry is added.
     * This method might be called multiple times with the same arguments and must therefore be implemented in
     * an idempotent way.
     *
     * @param participantId the participantId of the added entry
     * @param entry the added entry itself
     */
    void entryAdded(String participantId, T entry);

    /**
     * Callback: Once a listener has added itself to the Directory, the Directory will call this method whenever
     * a entry has been removed.
     * This method might be called multiple times with the same arguments and must therefore be implemented in
     * an idempotent way.
     *
     * @param participantId he participantId of the removed entry
     */
    void entryRemoved(String participantId);

}
