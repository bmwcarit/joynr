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
import org.franca.core.franca.FBasicTypeId

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
	«IF !attribute.isArray && attribute.type.predefined != null»
		«val type = attribute.type.predefined»
		«IF type==FBasicTypeId.STRING»
		//	«attributeType» result = "Hello World";
		«ELSEIF type==FBasicTypeId.BOOLEAN»
		//	«attributeType» result = false;
		«ELSEIF type==FBasicTypeId.INT8   ||
				type==FBasicTypeId.UINT8  ||
				type==FBasicTypeId.INT16  ||
				type==FBasicTypeId.UINT16 ||
				type==FBasicTypeId.INT32  ||
				type==FBasicTypeId.UINT32 ||
				type==FBasicTypeId.INT64  ||
				type==FBasicTypeId.UINT64»
		//	«attributeType» result = 42;
		«ELSEIF type==FBasicTypeId.DOUBLE   ||
				type==FBasicTypeId.FLOAT»
		//	«attributeType» result = 3.1415;
		«ELSE»
		//	«attributeType» result;
		«ENDIF»
	«ELSE»
	//	«attributeType» result;
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
			«IF !argument.isArray && argument.type.predefined != null»
				«val type = argument.type.predefined»
				«IF type==FBasicTypeId.STRING»
					«outputParamType» «argument.joynrName» = "Hello World";
				«ELSEIF type==FBasicTypeId.BOOLEAN»
					«outputParamType» «argument.joynrName» = false;
				«ELSEIF type==FBasicTypeId.INT8   ||
						type==FBasicTypeId.UINT8  ||
						type==FBasicTypeId.INT16  ||
						type==FBasicTypeId.UINT16 ||
						type==FBasicTypeId.INT32  ||
						type==FBasicTypeId.UINT32 ||
						type==FBasicTypeId.INT64  ||
						type==FBasicTypeId.UINT64»
					«outputParamType» «argument.joynrName» = 42;
				«ELSEIF type==FBasicTypeId.DOUBLE   ||
						type==FBasicTypeId.FLOAT»
					«outputParamType» «argument.joynrName» = 3.1415;
				«ELSE»
					«outputParamType» «argument.joynrName»;
				«ENDIF»
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