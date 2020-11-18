package io.joynr.generator.cpp.filter
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.BroadcastTemplate
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface

class FilterParameterTemplate implements BroadcastTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension NamingUtil
	@Inject extension BroadcastUtil

	override generate(FInterface serviceInterface, FBroadcast broadcast, boolean generateVersion)
'''
«val broadcastName =  broadcast.joynrName»
«val className = serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilterParameters"»
«val headerGuard = ("GENERATED_FILTER_PARAMETERS_"+getPackagePathWithJoynrPrefix(broadcast, "_", generateVersion)+"_"+broadcastName+"_H").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/BroadcastFilterParameters.h"
#include <string>
«getDllExportIncludeStatement()»

«getNamespaceStarter(serviceInterface, generateVersion)»
/**
 * @brief BroadcastFilterParameter class for interface «serviceInterface.joynrName.toFirstUpper»
 * broadcast «broadcastName»
 */
class «getDllExportMacro()» «className» : public BroadcastFilterParameters {
public:

	using BroadcastFilterParameters::BroadcastFilterParameters;

	«IF (getFilterParameters(broadcast).isEmpty())»
		/**
		 * @brief Sets value for given key
		 * @param key The key
		 * @param value The new value
		 */
		void set(std::string key, std::string value) {
			setFilterParameter(key, value);
		}

		/**
		 * @brief Gets value for given key
		 * @param key The key for which value should be retrieved
		 * @return The current value
		 */
		std::string get(std::string key) const {
			return getFilterParameter(key);
		}
	«ELSE»
		«FOR parameter: getFilterParameters(broadcast)»
			/**
			 * @brief Sets «parameter.toFirstUpper»
			 * @param value The new value
			 */
			void set«parameter.toFirstUpper»(std::string value) {
				setFilterParameter("«parameter»", value);
			}

			/**
			 * @brief Gets «parameter.toFirstUpper»
			 * @return The current value of «parameter.toFirstUpper»
			 */
			std::string get«parameter.toFirstUpper»() const {
				return getFilterParameter("«parameter»");
			}
		«ENDFOR»
	«ENDIF»
};

«getNamespaceEnder(serviceInterface, generateVersion)»

#endif // «headerGuard»
'''
}
