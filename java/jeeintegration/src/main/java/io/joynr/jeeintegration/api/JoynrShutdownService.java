/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.jeeintegration.api;

/**
 * Inject this service into your application in order to programmatically control shutdown of joynr runtime, for
 * example if you want to perform clean-up operations before the application quits.
 *
 * You can optionally first call {@link #prepareForShutdown()} in order to give joynr a chance to finish processing
 * messages and stop receiving any more incoming messages. This call will block until either the preparation is
 * complete, or a timeout occurs. See the joynr Java settings guide for details on how to configure the timeout.
 * Once you've made the prepareForShutdown call, you will not be able to send any more requests which require state
 * to be held by joynr (e.g. method calls via a Sync interface). You will be able to still perform calls for
 * stateless async and fire-and-forget methods, as these do not require any state to be held by the runtime.
 *
 * Thereafter you can call {@link #shutdown()} in order to tell the joynr runtime to close and free all resources
 * and perform any necessary clean-up on its part.
 */
public interface JoynrShutdownService {

    /**
     * Call this method to cause the joynr runtime to prepare for shutdown and perform initial clean-up. This method
     * blocks until completion or it times out. After calling this method you can only make requests via joynr which
     * to not require state to be held (e.g. stateless async or fire-and-forget).
     */
    void prepareForShutdown();

    /**
     * This call will completely shut down the joynr runtime and free up any and all resources it holds. You cannot
     * perform any further joynr operations after this method has been called.
     */
    void shutdown();
}
