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

class InterfaceProviderCppTemplate {

	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions
	
	def generate(FInterface serviceInterface)
	'''
		«warning()»
		«val interfaceName = serviceInterface.name.toFirstUpper»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"
		#include "joynr/InterfaceRegistrar.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"
		#include "joynr/RequestStatus.h"


		«getNamespaceStarter(serviceInterface)»
		«interfaceName»Provider::«interfaceName»Provider(const joynr::types::ProviderQos &providerQos) :
			I«serviceInterface.name.toFirstUpper»Base(),
			«FOR attribute: getAttributes(serviceInterface)»
				«attribute.name»(),
			«ENDFOR»
		    subscriptionManager(NULL),
		    domain(),
		    interfaceName(),
		    providerQos(providerQos)
		{
			// Register a request interpreter to interpret requests to this interface
			joynr::InterfaceRegistrar::instance().registerRequestInterpreter<«interfaceName»RequestInterpreter>(getInterfaceName());
		}
		
		«serviceInterface.name.toFirstUpper»Provider::~«serviceInterface.name.toFirstUpper»Provider()
		{
			// Unregister the request interpreter
			joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(getInterfaceName());
		}
		
		void «serviceInterface.name.toFirstUpper»Provider::setSubscriptionManager(joynr::SubscriptionManager* subscriptionManager) {
		    this->subscriptionManager = subscriptionManager;
		}
		
		void «serviceInterface.name.toFirstUpper»Provider::setDomainAndInterface(const QString &domain, const QString &interfaceName) {
		    this->domain = domain;
		    this->interfaceName = interfaceName;
		}
		
		joynr::types::ProviderQos «serviceInterface.name.toFirstUpper»Provider::getProviderQos() const {
		    return providerQos;
		}
		
		«FOR attribute: getAttributes(serviceInterface)»
			«var attributeType = getMappedDatatypeOrList(attribute)»
			void «serviceInterface.name.toFirstUpper»Provider::get«attribute.name.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, «attributeType»& result) {
			    result = «attribute.name»;
			    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
			}
			
			void «serviceInterface.name.toFirstUpper»Provider::set«attribute.name.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, const «attributeType»& «attribute.name») {
			    «attribute.name»Changed(«attribute.name»); 
			    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
			}

			void «serviceInterface.name.toFirstUpper»Provider::«attribute.name»Changed(const «attributeType»& «attribute.name») {
			    if(this->«attribute.name» == «attribute.name») {
			        // the value didn't change, no need for notification
			        return;
			    }
			    this->«attribute.name» = «attribute.name»;
			    onAttributeValueChanged("«attribute.name»", QVariant::fromValue(«attribute.name»));
			}
		«ENDFOR»
		«getNamespaceEnder(serviceInterface)»
	'''
}