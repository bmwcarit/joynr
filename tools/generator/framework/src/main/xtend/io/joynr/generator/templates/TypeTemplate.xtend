package io.joynr.generator.templates;

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

import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection

/*
 * This is the base class for all generation templates which process a Franca type
 */
abstract class TypeTemplate {
	protected var majorVersion = 0
	protected var minorVersion = 0

	new(FType type) {
		if (type.eContainer instanceof FTypeCollection) {
			val interface = type.eContainer as FTypeCollection
			if (interface.version !== null) {
				majorVersion = interface.version.major
				minorVersion = interface.version.minor
			}
		}
	}

	def CharSequence generate(boolean generateVersion)
}
