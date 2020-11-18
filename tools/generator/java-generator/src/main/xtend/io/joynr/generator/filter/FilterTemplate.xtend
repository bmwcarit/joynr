package io.joynr.generator.filter
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
import io.joynr.generator.templates.BroadcastTemplate
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface

class FilterTemplate implements BroadcastTemplate{
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension NamingUtil
	@Inject extension TemplateBase

	override generate(FInterface serviceInterface, FBroadcast broadcast, boolean generateVersion) {
		val broadcastName =  broadcast.joynrName
		val className = serviceInterface.joynrName + broadcastName.toFirstUpper + "BroadcastFilter"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".", generateVersion)

		'''
		«warning()»
		package «packagePath»;

		import io.joynr.pubsub.publication.BroadcastFilterImpl;
		«FOR datatype: getRequiredIncludesFor(broadcast, generateVersion)»
		import «datatype»;
		«ENDFOR»

		public abstract class «className» extends BroadcastFilterImpl {
			public «className»(){
				super("«broadcastName»");
			}

			/*
			* Override this method to provide a filter logic implementation.
			*/
			public abstract boolean filter(
					«broadcast.commaSeperatedTypedOutputParameterListLinebreak»,
					«serviceInterface.joynrName»BroadcastInterface.«broadcastName.toFirstUpper»BroadcastFilterParameters filterParameters);
		};

		'''
	}
}