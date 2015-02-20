package io.joynr.generator.cpp.filter
/*
 * !!!
 *
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import io.joynr.generator.util.BroadcastTemplate

class FilterParameterTemplate implements BroadcastTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FInterface serviceInterface, FBroadcast broadcast) {
		val broadcastName =  broadcast.joynrName
		val className = serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilterParameters"
		val headerGuard = ("GENERATED_FILTER_PARAMETERS_"+getPackagePathWithJoynrPrefix(broadcast, "_")+"_"+broadcastName+"_H").toUpperCase
		'''
		«warning()»

		#ifndef «headerGuard»
		#define «headerGuard»

		#include "joynr/BroadcastFilterParameters.h"
		«getDllExportIncludeStatement()»

		«getNamespaceStarter(serviceInterface)»
		class «getDllExportMacro()» «className» : public BroadcastFilterParameters {
		public:

			«IF (getFilterParameters(broadcast).isEmpty())»
				void set(QString key, QString value) {
					setFilterParameter(key, value);
				}

				QString get(QString key) const {
					return getFilterParameter(key);
				}
			«ELSE»
				«FOR parameter: getFilterParameters(broadcast)»
					void set«parameter.toFirstUpper»(QString value) {
						setFilterParameter("«parameter»", value);
					}
					QString get«parameter.toFirstUpper»() const {
						return getFilterParameter("«parameter»");
					}
				«ENDFOR»
			«ENDIF»
		};

		«getNamespaceEnder(serviceInterface)»

		#endif // «headerGuard»
		'''
	}


}