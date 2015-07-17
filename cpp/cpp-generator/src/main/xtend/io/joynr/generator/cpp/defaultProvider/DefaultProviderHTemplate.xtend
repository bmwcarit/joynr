package io.joynr.generator.cpp.defaultProvider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.generator.cpp.util.CppMigrateToStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface

class DefaultProviderHTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppMigrateToStdTypeUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_Default"+interfaceName+"Provider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/joynrlogging.h"

«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include "«parameterType»"
«ENDFOR»

«getIncludesFor(getAllPrimitiveTypes(serviceInterface))»

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»AbstractProvider.h"

«getNamespaceStarter(serviceInterface)»

class «getDllExportMacro()» Default«interfaceName»Provider: public «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»AbstractProvider {

public:
	Default«interfaceName»Provider();

	virtual ~Default«interfaceName»Provider();

	«FOR attribute: getAttributes(serviceInterface)»
		// Only use this for pulling providers, not for pushing providers
		//	void get«attribute.joynrName.toFirstUpper»(
		//			std::future<void(
		//					const joynr::RequestStatus&,
		//					const «attribute.typeName»)> callbackFct);
	«ENDFOR»

	«FOR method: getMethods(serviceInterface)»
		«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
		«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method)»
		void «method.joynrName»(
				«IF !method.inputParameters.empty»«inputTypedParamList»,«ENDIF»
				std::function<void(
						const joynr::RequestStatus& joynrInternalStatus«IF !method.outputParameters.empty»,«ENDIF»
						«outputTypedParamList»)> callbackFct);
	«ENDFOR»

private:
	static joynr::joynr_logging::Logger* logger;

};

«getNamespaceEnder(serviceInterface)»

#endif // «headerGuard»
'''
}
