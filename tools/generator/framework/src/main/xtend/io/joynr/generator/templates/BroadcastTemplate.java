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
package io.joynr.generator.templates;

import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FInterface;

/*
 * This interface shall be used by all generation templates which process a Franca broadcast type
 */
public interface BroadcastTemplate {

    public CharSequence generate(FInterface serviceInterface, FBroadcast broadcast, boolean generateVersion);
}
