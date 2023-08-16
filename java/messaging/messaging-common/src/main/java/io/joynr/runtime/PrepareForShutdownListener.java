/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.runtime;

public interface PrepareForShutdownListener {
    /**
     * This method is called just before the system shuts down in order to give components a chance to finish
     * essential operations before the actual {@link io.joynr.runtime.ShutdownListener#shutdown()} is performed. Implementations should block until
     * they're finished, but should also make sure to timeout after a few seconds if they can't finish quickly.
     */
    void prepareForShutdown();
}
