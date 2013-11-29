package io.joynr.generator.cpp.defaultProvider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

class DefaultProviderCppTemplate {
	
	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions
		
	def generate(FInterface serviceInterface){
		val interfaceName = serviceInterface.joynrName
		'''
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
			//void Default«interfaceName»Provider::get«attributename.toFirstUpper»(RequestStatus& status, «getMappedDatatypeOrList(attribute)»& result) {
			// LOG_WARN(logger, "**********************************************");
			// LOG_WARN(logger, "* Default«interfaceName»Provider::get«attributename.toFirstUpper» called");
			// LOG_WARN(logger, "**********************************************");
			«IF attributeType=="QString"»
				//		result = "Hello World";
			«ELSEIF attributeType=="bool"»
				//		result = false;
			«ELSEIF attributeType=="int"»
				//		result = 42;
			«ELSEIF attributeType=="double"»
				//		result = 3.1415;
			«ELSE»
				//		result = «attributeType»(); 
			«ENDIF»
			//    status.setCode(RequestStatusCode::OK);
			//}
			
		«ENDFOR»
		«FOR method: getMethods(serviceInterface)»
			«val methodName = method.joynrName»
			«val outputParameterType = getMappedOutputParameter(method)»
			«IF outputParameterType.head =="void"»
				void  Default«interfaceName»Provider::«methodName»(joynr::RequestStatus& status«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))» ) {
			«ELSE»
				void  Default«interfaceName»Provider::«methodName»(joynr::RequestStatus& status, «outputParameterType.head»& result«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») {
				«IF outputParameterType.head=="QString"»
					result = "Hello World";
				«ELSEIF outputParameterType.head=="bool"»
					result = false;
				«ELSEIF outputParameterType.head=="int"»
					result = 42;
				«ELSEIF outputParameterType.head=="double"»
				    result = 3.1415;
				«ELSE»
				    Q_UNUSED(result);
				«ENDIF»
			«ENDIF»
				«FOR inputParameter: getInputParameters(method)»
					Q_UNUSED(«inputParameter.joynrName»);
				«ENDFOR»
				LOG_WARN(logger, "**********************************************");
				LOG_WARN(logger, "* Default«interfaceName»Provider::«methodName» called");
				LOG_WARN(logger, "**********************************************");	
				status.setCode(joynr::RequestStatusCode::OK);
			}
			
		«ENDFOR»    
		«getNamespaceEnder(serviceInterface)»
		'''
	}
	
	/**
	 * add to line 73
	 * 
	 * 	«ELSEIF isArray(getOutputParameter(method))»
	 *	result = QList<«getMappedDatatype(getOutputParameter(method))»>();
	 * 
	 */
}