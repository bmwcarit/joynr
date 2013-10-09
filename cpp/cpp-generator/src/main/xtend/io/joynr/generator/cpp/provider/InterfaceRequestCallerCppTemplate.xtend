package io.joynr.generator.cpp.provider
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

class InterfaceRequestCallerCppTemplate {

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	def generate(FInterface serviceInterface)
	'''
		«warning()»
		
		
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«serviceInterface.name.toFirstUpper»RequestCaller.h"
		«FOR datatype: getRequiredIncludesFor(serviceInterface)»
			#include "«datatype»"
		«ENDFOR»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«serviceInterface.name.toFirstUpper»Provider.h"
		
		«getNamespaceStarter(serviceInterface)»
		«serviceInterface.name.toFirstUpper»RequestCaller::«serviceInterface.name.toFirstUpper»RequestCaller(QSharedPointer<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«serviceInterface.name.toFirstUpper»Provider> provider)
		    : joynr::RequestCaller(I«serviceInterface.name.toFirstUpper»::getInterfaceName()),
		      provider(provider)
		{
		}
		
		«FOR attribute: getAttributes(serviceInterface)»
		   	void «serviceInterface.name.toFirstUpper»RequestCaller::get«attribute.name.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, «getMappedDatatypeOrList(attribute)» &«attribute.name»){
		   		provider->get«attribute.name.toFirstUpper»(joynrInternalStatus, «attribute.name»); 
		   	}

		   	void «serviceInterface.name.toFirstUpper»RequestCaller::set«attribute.name.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, const «getMappedDatatypeOrList(attribute)»& «attribute.name»){
		   		provider->set«attribute.name.toFirstUpper»(joynrInternalStatus, «attribute.name»);
		   	}

		«ENDFOR»
		«FOR method: getMethods(serviceInterface)»
			«val outputParameterType = getMappedOutputParameter(method)»
			«IF outputParameterType.head=="void"»
				void «serviceInterface.name.toFirstUpper»RequestCaller::«method.name»(joynr::RequestStatus& status«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))» ){
					provider->«method.name»(status«prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»); 
				}
			«ELSE»
				void «serviceInterface.name.toFirstUpper»RequestCaller::«method.name»(joynr::RequestStatus& joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»){
					provider->«method.name»(joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedUntypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»); 
				}
			«ENDIF»

		«ENDFOR»
		
		void «serviceInterface.name.toFirstUpper»RequestCaller::registerAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener)
		{
			provider->registerAttributeListener(attributeName, attributeListener);
		}
		
		void «serviceInterface.name.toFirstUpper»RequestCaller::unregisterAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener)
		{
			provider->unregisterAttributeListener(attributeName, attributeListener);
		}
		
		«getNamespaceEnder(serviceInterface)»
	'''
}
