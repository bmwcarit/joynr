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

class InterfaceRequestCallerHTemplate {

	@Inject
	private extension TemplateBase
	
	@Inject
	private extension JoynrCppGeneratorExtensions

	def generate(FInterface serviceInterface) {
		val interfaceName = serviceInterface.joynrName
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_"+interfaceName+"RequestCaller_h").toUpperCase
		'''
		«warning()»
		
		#ifndef «headerGuard»
		#define «headerGuard»

		#include "joynr/PrivateCopyAssign.h"
		«getDllExportIncludeStatement()»
		#include "joynr/RequestCaller.h"
		#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
		#include <QSharedPointer>
		#include <QList>
		#include <QVariant>
		
		«getNamespaceStarter(serviceInterface)»
	
		class «interfaceName»Provider;

		class «getDllExportMacro()» «interfaceName»RequestCaller : public joynr::RequestCaller, public «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::I«interfaceName»Sync {
		public:
		    explicit «interfaceName»RequestCaller(QSharedPointer<«interfaceName»Provider> provider);
		
		    virtual ~«interfaceName»RequestCaller(){}
		    
		    «FOR attribute: getAttributes(serviceInterface)»
		    «var attributeName = attribute.joynrName»
		    	virtual void get«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, «getMappedDatatypeOrList(attribute)» &«attributeName»);
		    	virtual void set«attributeName.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, const «getMappedDatatypeOrList(attribute)»& «attributeName»);
		    	
			«ENDFOR»
			«FOR method: getMethods(serviceInterface)»
				«val outputParameterType = getMappedOutputParameter(method).head»
				«var methodName = method.joynrName»
				«IF outputParameterType=="void"»
					virtual void  «methodName»(joynr::RequestStatus& joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))» );
				«ELSE»				
					virtual void  «methodName»(joynr::RequestStatus& joynrInternalStatus«prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»);
				«ENDIF»
			«ENDFOR»
			«FOR broadcast: serviceInterface.broadcasts»
				virtual void get«broadcast.name.toFirstUpper»(joynr::RequestStatus& joynrInternalStatus, QList<QVariant>& result);
			«ENDFOR»

			void registerAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener);
			void unregisterAttributeListener(const QString& attributeName, joynr::IAttributeListener* attributeListener);

			void registerBroadcastListener(const QString& broadcastName, joynr::IBroadcastListener* broadcastListener);
			void unregisterBroadcastListener(const QString& broadcastName, joynr::IBroadcastListener* broadcastListener);
			
		private:
		    DISALLOW_COPY_AND_ASSIGN(«interfaceName»RequestCaller);
		    QSharedPointer<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider;
		};
		
		«getNamespaceEnder(serviceInterface)»
		#endif // «headerGuard»
		'''
	}
}
