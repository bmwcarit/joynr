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
import org.franca.core.franca.FInterface
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate

class DefaultProviderCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

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
	«var attributeType = getMappedDatatypeOrList(attribute)»
	// Only use this for pulling providers, not for pushing providers
	//	void Default«interfaceName»Provider::get«attributename.toFirstUpper»(
	//		std::function<void(
	//				const joynr::RequestStatus&,
	//				const «getMappedDatatypeOrList(attribute)»&)> callbackFct) {
	// LOG_WARN(logger, "**********************************************");
	// LOG_WARN(logger, "* Default«interfaceName»Provider::get«attributename.toFirstUpper» called");
	// LOG_WARN(logger, "**********************************************");
	«IF attributeType=="QString"»
		//		«getMappedDatatypeOrList(attribute)» result = "Hello World";
	«ELSEIF attributeType=="bool"»
		//		«getMappedDatatypeOrList(attribute)» result = false;
	«ELSEIF attributeType=="int"»
		//		«getMappedDatatypeOrList(attribute)» result = 42;
	«ELSEIF attributeType=="double"»
		//		«getMappedDatatypeOrList(attribute)» result = 3.1415;
	«ELSE»
		//		«getMappedDatatypeOrList(attribute)» result = «attributeType»();
	«ENDIF»
	//	callbackFct(joynr::RequestStatus(joynr::RequestStatusCode::OK), result);
	//}

«ENDFOR»
«FOR method: getMethods(serviceInterface)»
	«val methodName = method.joynrName»
	«val outputTypedParamList = if (method.outputParameters.empty) "" else ("const " + method.outputParameters.head.mappedDatatypeOrList + "& " + method.outputParameters.head.joynrName)»
	«val outputUntypedParamList = method.getCommaSeperatedUntypedOutputParameterList»
	«val inputTypedParamList = getCommaSeperatedTypedInputParameterList(method)»
	void Default«interfaceName»Provider::«method.joynrName»(
			«IF !method.inputParameters.empty»«inputTypedParamList»,«ENDIF»
			std::function<void(
					const joynr::RequestStatus& joynrInternalStatus«IF !method.outputParameters.empty»,«ENDIF»
					«outputTypedParamList»)> callbackFct) {
		«FOR inputParameter: getInputParameters(method)»
			Q_UNUSED(«inputParameter.joynrName»);
		«ENDFOR»
		«IF !method.outputParameters.empty»
			«val argument = method.outputParameters.head»
			«val outputParamType = argument.getMappedDatatypeOrList»
			«IF outputParamType=="QString"»
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
		«ENDIF»
		LOG_WARN(logger, "**********************************************");
		LOG_WARN(logger, "* Default«interfaceName»Provider::«methodName» called");
		LOG_WARN(logger, "**********************************************");
		callbackFct(
				joynr::RequestStatus(joynr::RequestStatusCode::OK)«IF !method.outputParameters.empty», «method.outputParameters.head.joynrName»«ENDIF»
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