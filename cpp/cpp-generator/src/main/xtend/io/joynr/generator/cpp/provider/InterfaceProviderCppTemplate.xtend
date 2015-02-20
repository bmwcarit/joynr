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

class InterfaceProviderCppTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions
	override generate(FInterface serviceInterface)
	'''
		«warning()»
		«val interfaceName = serviceInterface.joynrName»
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"
		#include "joynr/InterfaceRegistrar.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"
		#include "joynr/RequestStatus.h"


		«getNamespaceStarter(serviceInterface)»
		«interfaceName»Provider::«interfaceName»Provider(const joynr::types::ProviderQos &providerQos) :
			I«interfaceName»Base(),
			«FOR attribute: getAttributes(serviceInterface)»
				«attribute.joynrName»(),
			«ENDFOR»
		    subscriptionManager(NULL),
		    domain(),
		    interfaceName(),
		    providerQos(providerQos)
		{
			// Register a request interpreter to interpret requests to this interface
			joynr::InterfaceRegistrar::instance().registerRequestInterpreter<«interfaceName»RequestInterpreter>(getInterfaceName());
		}
		
		«interfaceName»Provider::~«interfaceName»Provider()
		{
			// Unregister the request interpreter
			joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(getInterfaceName());
		}
		
		void «interfaceName»Provider::setSubscriptionManager(joynr::SubscriptionManager* subscriptionManager) {
		    this->subscriptionManager = subscriptionManager;
		}
		
		void «interfaceName»Provider::setDomainAndInterface(const QString &domain, const QString &interfaceName) {
		    this->domain = domain;
		    this->interfaceName = interfaceName;
		}
		
		joynr::types::ProviderQos «interfaceName»Provider::getProviderQos() const {
		    return providerQos;
		}
		
		«FOR attribute: getAttributes(serviceInterface)»
			«var attributeType = getMappedDatatypeOrList(attribute)»
			«var attributeName = attribute.joynrName»
			void «interfaceName»Provider::get«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, «attributeType»& result) {
			    result = «attributeName»;
			    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
			}
			
			void «interfaceName»Provider::set«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, const «attributeType»& «attributeName») {
			    «attributeName»Changed(«attributeName»); 
			    joynrInternalStatus.setCode(joynr::RequestStatusCode::OK);
			}

			void «interfaceName»Provider::«attributeName»Changed(const «attributeType»& «attributeName») {
			    if(this->«attributeName» == «attributeName») {
			        // the value didn't change, no need for notification
			        return;
			    }
			    this->«attributeName» = «attributeName»;
			    onAttributeValueChanged("«attributeName»", QVariant::fromValue(«attributeName»));
			}
		«ENDFOR»

		«FOR broadcast: serviceInterface.broadcasts»
			«var broadcastName = broadcast.joynrName»
			void «interfaceName»Provider::fire«broadcastName.toFirstUpper»(«getMappedOutputParametersCommaSeparated(broadcast, true)») {
			    QList<QVariant> broadcastValues;
				«FOR parameter: getOutputParameters(broadcast)»
				    broadcastValues.append(QVariant::fromValue(«parameter.name»));
				«ENDFOR»
			    fireBroadcast("«broadcastName»", broadcastValues);
			}
		«ENDFOR»
		«getNamespaceEnder(serviceInterface)»
	'''
}