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

class InterfaceProviderHTemplate {
	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	def generate(FInterface serviceInterface) {
		val interfaceName = serviceInterface.joynrName
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_"+interfaceName+"Provider_h").toUpperCase
	'''
	«warning()»
	#ifndef «headerGuard»
	#define «headerGuard»

	#include "joynr/PrivateCopyAssign.h"

	#include "joynr/Provider.h"
	#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
	#include "joynr/DeclareMetatypeUtil.h"
	#include "joynr/types/ProviderQos.h"
	#include "joynr/RequestCallerFactory.h"
	#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"

	«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
		#include "«parameterType»"
	«ENDFOR»

	«getDllExportIncludeStatement()»

	namespace joynr { class SubscriptionManager; }

	«getNamespaceStarter(serviceInterface)»

	class «getDllExportMacro()» «interfaceName»Provider : public «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::I«interfaceName»Sync, public joynr::Provider {

	public:
	    //TODO remove default value for ProviderQos and pass in real qos parameters
	    «interfaceName»Provider(const joynr::types::ProviderQos& providerQos);
	    //for each Attribute the provider needs setters, sync and async getters.
	    //They have default implementation for pushing Providers and can be overwritten by pulling Providers.
	    virtual ~«interfaceName»Provider();

	    // request status, result, (params......)*

		«FOR attribute: getAttributes(serviceInterface)»
			«var attributeName = attribute.joynrName»
			virtual void get«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, «getMappedDatatypeOrList(attribute)»& result);
			virtual void set«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, const «getMappedDatatypeOrList(attribute)»& «attributeName»);
			/**
			* @brief «attributeName»Changed must be called by a concrete provider to signal attribute
			* modifications. It is used to implement onchange subscriptions.
			* @param «attributeName» the new attribute value
			*/
			void «attributeName»Changed(const «getMappedDatatypeOrList(attribute)»& «attributeName»);
		«ENDFOR»
		«FOR method: getMethods(serviceInterface)»
			«val outputParameterType = getMappedOutputParameter(method)»
			«IF outputParameterType.head=="void"»
				virtual void «method.name»(joynr::RequestStatus& joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») = 0;
			«ELSE»
				virtual void «method.name»(joynr::RequestStatus& joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))») = 0;
			«ENDIF»
		«ENDFOR»

		«FOR broadcast: serviceInterface.broadcasts»
			«var broadcastName = broadcast.joynrName»
			/**
			* @brief «broadcastName»EventOccured must be called by a concrete provider to signal an occured
			* event. It is used to implement broadcast publications.
			* @param «broadcastName» the new attribute value
			*/
			void «broadcastName»EventOccured(«getMappedOutputParametersCommaSeparated(broadcast, true)»);
			
			virtual void get«broadcastName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, QList<QVariant>& result);
		«ENDFOR»

	    void setSubscriptionManager(joynr::SubscriptionManager* subscriptionManager);
	    void setDomainAndInterface(const QString& domain, const QString& interfaceName);

	    joynr::types::ProviderQos getProviderQos() const;

	protected:
		«FOR attribute: getAttributes(serviceInterface)»
		    «getMappedDatatypeOrList(attribute)» «attribute.joynrName»;
		«ENDFOR»
		
		«FOR broadcast: serviceInterface.broadcasts»
			struct «broadcast.joynrName.toFirstUpper»OutputParameters {
			«FOR parameter: getOutputParameters(broadcast)»
			    «getMappedDatatypeOrList(parameter)» «parameter.name»;
			«ENDFOR»
			};
			«broadcast.joynrName.toFirstUpper»OutputParameters «broadcast.joynrName.toFirstLower»OutputParameters;
			
		«ENDFOR»

	private:
	    DISALLOW_COPY_AND_ASSIGN(«interfaceName»Provider);
	    joynr::SubscriptionManager* subscriptionManager;
	    QString domain;
	    QString interfaceName;
	    joynr::types::ProviderQos providerQos;
	};
	«getNamespaceEnder(serviceInterface)»

	namespace joynr {

	// Helper class for use by the RequestCallerFactory.
	// This class creates instances of «interfaceName»RequestCaller
	template<>
	class RequestCallerFactoryHelper<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> {
	public:
		QSharedPointer<joynr::RequestCaller> create(QSharedPointer<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider) {
			return QSharedPointer<joynr::RequestCaller>(new «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»RequestCaller(provider));
	    }
	};
	} // namespace joynr	

	#endif // «headerGuard»
	'''
	}
}
