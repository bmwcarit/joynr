package io.joynr.generator.cpp.proxy
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import org.franca.core.franca.FInterface
import io.joynr.generator.util.InterfaceTemplate

class InterfaceSyncProxyCppTemplate  implements InterfaceTemplate{
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FInterface fInterface) {
		val interfaceName =  fInterface.joynrName
		val className = interfaceName + "Proxy"
		val syncClassName = interfaceName + "SyncProxy"
		'''
		«warning()»
		
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«syncClassName».h"
		#include "joynr/Request.h"
		#include "joynr/Reply.h"
		#include "joynr/Dispatcher.h"
		#include "joynr/DispatcherUtils.h"
		#include "joynr/exceptions.h"
		#include "joynr/RequestStatus.h"

		«FOR datatype: getRequiredIncludesFor(fInterface)»
			#include "«datatype»"
		«ENDFOR»
		
		«getNamespaceStarter(fInterface)» 
		// The proxies will contain all arbitration checks
		// the connectors will contain the JSON related code
		
		«syncClassName»::«syncClassName»(
		        QSharedPointer<joynr::system::Address> messagingAddress,
		        joynr::ConnectorFactory* connectorFactory,
		        joynr::IClientCache *cache,
		        const QString &domain,
		        const joynr::ProxyQos &proxyQos,
		        const joynr::MessagingQos &qosSettings,
		        bool cached
		) :
		        joynr::ProxyBase(connectorFactory, cache, domain, getInterfaceName(), proxyQos, qosSettings, cached),
		        «className»Base(messagingAddress, connectorFactory, cache, domain, proxyQos, qosSettings, cached)
		{
		}

		«FOR attribute: getAttributes(fInterface)»
			«var attributeName = attribute.joynrName»
			«var attributeType = getMappedDatatypeOrList(attribute)» 
			«var getAttribute = "get" + attributeName.toFirstUpper» 
			«var setAttribute = "set" + attributeName.toFirstUpper» 
			«IF attribute.readable»
			void «syncClassName»::«getAttribute»(joynr::RequestStatus& status, «attributeType»& result)
			{
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «getAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«getAttribute»(status, result);
			    }
			}
			«ENDIF»

			«IF attribute.writable»
			void «syncClassName»::«setAttribute»(joynr::RequestStatus& status, const «attributeType»& value)
			{
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «setAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«setAttribute»(status, value);
			    }
			}
			«ENDIF»
			
		«ENDFOR»
		«FOR method: getMethods(fInterface)»
			«var methodName = method.name»
			«var paramsSignature = prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			«var params = prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»
			/*
			 * «methodName»
			 */
			«IF getMappedOutputParameter(method).head=="void"»
			    void «syncClassName»::«methodName»(joynr::RequestStatus& status «paramsSignature»)
			    {
			        if (connector==NULL){
			            LOG_WARN(logger, "proxy cannot invoke «methodName» because the communication end partner is not (yet) known");
			        }
			        else{
			            connector->«methodName»(status «params»);
			        }
			    }	
			«ELSE»
			    void «syncClassName»::«methodName»(joynr::RequestStatus& status, «getMappedOutputParameter(method).head» &result «paramsSignature»)
			    {
			        if (connector==NULL){
			            LOG_WARN(logger, "proxy cannot invoke «methodName» because the communication end partner is not (yet) known");
			        }
			        else{
			            connector->«methodName»(status, result «params»);
			        }
			    }
		    «ENDIF»

		«ENDFOR»
		«getNamespaceEnder(fInterface)»
		'''
	}	
}
