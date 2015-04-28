package io.joynr.generator.cpp.provider
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

class InterfaceRequestCallerCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FInterface serviceInterface){
	var interfaceName = serviceInterface.joynrName;
	'''
		«warning()»

		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"
		«FOR datatype: getRequiredIncludesFor(serviceInterface)»
			#include "«datatype»"
		«ENDFOR»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"

		«getNamespaceStarter(serviceInterface)»
		«interfaceName»RequestCaller::«interfaceName»RequestCaller(QSharedPointer<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider)
		    : joynr::RequestCaller(I«interfaceName»::getInterfaceName()),
		      provider(provider)
		{
		}

		«FOR attribute: getAttributes(serviceInterface)»
			«var attributeName = attribute.joynrName»
			void «interfaceName»RequestCaller::get«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, «getMappedDatatypeOrList(attribute)» &«attributeName»){
				provider->get«attributeName.toFirstUpper»(joynrInternalStatus, «attributeName»);
			}

			void «interfaceName»RequestCaller::set«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, const «getMappedDatatypeOrList(attribute)»& «attributeName»){
				provider->set«attributeName.toFirstUpper»(joynrInternalStatus, «attributeName»);
			}

		«ENDFOR»
		«FOR method: getMethods(serviceInterface)»
			«val outputParameterType = getMappedOutputParameter(method)»
			«val methodName = method.joynrName»
			«IF outputParameterType.head=="void"»
				void «interfaceName»RequestCaller::«methodName»(joynr::RequestStatus& status«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))» ){
					provider->«methodName»(status«prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»);
				}
			«ELSE»
				void «interfaceName»RequestCaller::«methodName»(joynr::RequestStatus& joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»){
					provider->«methodName»(joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedUntypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»);
				}
			«ENDIF»

		«ENDFOR»

		void «interfaceName»RequestCaller::registerAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener)
		{
			provider->registerAttributeListener(attributeName, attributeListener);
		}

		void «interfaceName»RequestCaller::unregisterAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener)
		{
			provider->unregisterAttributeListener(attributeName, attributeListener);
		}

		void «interfaceName»RequestCaller::registerBroadcastListener(const QString& broadcastName, joynr::IBroadcastListener* broadcastListener)
		{
			provider->registerBroadcastListener(broadcastName, broadcastListener);
		}

		void «interfaceName»RequestCaller::unregisterBroadcastListener(const QString& broadcastName, joynr::IBroadcastListener* broadcastListener)
		{
			provider->unregisterBroadcastListener(broadcastName, broadcastListener);
		}

		«getNamespaceEnder(serviceInterface)»
	'''
	}
}
