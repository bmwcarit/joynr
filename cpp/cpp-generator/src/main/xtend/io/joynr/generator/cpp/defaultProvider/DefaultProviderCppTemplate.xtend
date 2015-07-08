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

class DefaultProviderCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppMigrateToStdTypeUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«warning()»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/Default«interfaceName»Provider.h"
#include "joynr/RequestStatus.h"
#include "joynr/joynrlogging.h"

«getNamespaceStarter(serviceInterface)»

using namespace joynr::joynr_logging;

Logger* Default«interfaceName»Provider::logger = Logging::getInstance()->getLogger("PROV", "Default«interfaceName»Provider");

Default«interfaceName»Provider::Default«interfaceName»Provider(const joynr::types::ProviderQos& providerQos) :
	«interfaceName»Provider(providerQos)
{
}

Default«interfaceName»Provider::~Default«interfaceName»Provider()
{
}

«FOR attribute: getAttributes(serviceInterface)»
	«val attributename = attribute.joynrName»
	«var attributeType = attribute.typeName»
	// Only use this for pulling providers, not for pushing providers
	//	void Default«interfaceName»Provider::get«attributename.toFirstUpper»(
	//		std::function<void(
	//				const joynr::RequestStatus&,
	//				const «attribute.typeName»&)> callbackFct) {
	// LOG_WARN(logger, "**********************************************");
	// LOG_WARN(logger, "* Default«interfaceName»Provider::get«attributename.toFirstUpper» called");
	// LOG_WARN(logger, "**********************************************");
	«IF attributeType=="std::string"»
		//		«attribute.typeName» result = "Hello World";
	«ELSEIF attributeType=="bool"»
		//		«attribute.typeName» result = false;
	«ELSEIF attributeType=="int"»
		//		«attribute.typeName» result = 42;
	«ELSEIF attributeType=="double"»
		//		«attribute.typeName» result = 3.1415;
	«ELSE»
		//		«attribute.typeName» result = «attributeType»();
	«ENDIF»
	//	callbackFct(joynr::RequestStatus(joynr::RequestStatusCode::OK), result);
	//}

«ENDFOR»
«FOR method: getMethods(serviceInterface)»
	«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
	«val outputUntypedParamList = getCommaSeperatedUntypedOutputParameterList(method)»
	«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method)»
	«val methodName = method.joynrName»
	void Default«interfaceName»Provider::«method.joynrName»(
			«IF !method.inputParameters.empty»«inputTypedParamList»,«ENDIF»
			std::function<void(
					const joynr::RequestStatus& joynrInternalStatus«IF !method.outputParameters.empty»,«ENDIF»
					«outputTypedParamList»)> callbackFct) {
		«FOR inputParameter: getInputParameters(method)»
			Q_UNUSED(«inputParameter.joynrName»);
		«ENDFOR»
		«FOR argument : method.outputParameters»
			«val outputParamType = argument.typeName»
			«IF outputParamType=="std::string"»
				«outputParamType» «argument.joynrName» = "Hello World";
			«ELSEIF outputParamType=="bool"»
				«outputParamType» «argument.joynrName» = false;
			«ELSEIF outputParamType=="int"»
				«outputParamType» «argument.joynrName» = 42;
			«ELSEIF outputParamType=="double"»
				«outputParamType» «argument.joynrName» = 3.1415;
			«ELSE»
				«outputParamType» «argument.joynrName»;
			«ENDIF»
		«ENDFOR»
		LOG_WARN(logger, "**********************************************");
		LOG_WARN(logger, "* Default«interfaceName»Provider::«methodName» called");
		LOG_WARN(logger, "**********************************************");
		callbackFct(
				joynr::RequestStatus(joynr::RequestStatusCode::OK)«IF !method.outputParameters.empty»,«ENDIF»
				«outputUntypedParamList»
		);
	}

«ENDFOR»
«getNamespaceEnder(serviceInterface)»
'''

	/**
	 * add to line 73
	 *
	 *	«ELSEIF isArray(getOutputParameter(method))»
	 *	result = QList<«getMappedDatatype(getOutputParameter(method))»>();
	 *
	 */
}