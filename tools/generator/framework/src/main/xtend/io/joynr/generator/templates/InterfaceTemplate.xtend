package io.joynr.generator.templates
/*
 * !!!
 *
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 *
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
 */

import com.google.inject.Inject
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.templates.util.JoynrGeneratorExtensions
import org.franca.core.franca.FInterface
import io.joynr.generator.templates.util.BroadcastUtil
import org.franca.core.franca.FBroadcast

/*
 * This is the base class used by all generation templates which process a Franca interface type
 */
abstract class InterfaceTemplate {
	@Inject protected extension JoynrGeneratorExtensions
	@Inject protected extension BroadcastUtil
	protected FInterface francaIntf
	protected var majorVersion = 0
	protected var minorVersion = 0

	def init() {
		if (francaIntf.version !== null) {
			majorVersion = francaIntf.version.major;
			minorVersion = francaIntf.version.minor;
		}
	}

	@Inject
	def setFrancaInterface(@Assisted FInterface francaIntf) {
		this.francaIntf = francaIntf
		init()
	}

	def CharSequence generate(boolean generateVersion)

	def hasSelectiveBroadcast() {
		francaIntf.broadcasts.exists[selective]
	}

	def hasNonSelectiveBroadcast() {
		francaIntf.broadcasts.exists[!selective]
	}

	def getBroadcastFilterClassName(FBroadcast broadcast) {
		getBroadcastFilterClassName(francaIntf, broadcast)
	}
}
