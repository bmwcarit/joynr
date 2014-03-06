package io.joynr.generator.cpp.proxy
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

class InterfaceAsyncProxyCppTemplate  {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	
	def generate(FInterface fInterface) {
		val interfaceName =  fInterface.joynrName
		val className = interfaceName + "Proxy"
		val asyncClassName = interfaceName + "AsyncProxy"
		'''
		«warning()»
		
		#include "«getPackagePathWithJoynrPrefix(fInterface, "/")»/«asyncClassName».h"
		«FOR datatype: getRequiredIncludesFor(fInterface)»
			#include "«datatype»"
		«ENDFOR»
		
		#include "joynr/exceptions.h"
		#include "joynr/Request.h"
		#include "joynr/Reply.h"
		#include <cassert>
		
		«getNamespaceStarter(fInterface)» 
		«asyncClassName»::«asyncClassName»(
				joynr::ICapabilities* capabilitiesStub,
				QSharedPointer<joynr::system::Address> messagingAddress,
				joynr::ConnectorFactory* connectorFactory,
				joynr::IClientCache *cache,
				const QString &domain,
		        const joynr::ProxyQos& proxyQos,
		        const joynr::MessagingQos &qosSettings,
		        bool cached
		) :
				joynr::ProxyBase(connectorFactory, cache, domain, getInterfaceName(), proxyQos, qosSettings, cached),
		      	«className»Base(capabilitiesStub, messagingAddress, connectorFactory, cache, domain, proxyQos, qosSettings, cached)
		{
		}
		
		«FOR attribute: getAttributes(fInterface)»
			«var attributeName = attribute.joynrName»
			«var attributeType = getMappedDatatypeOrList(attribute)» 
            «var getAttribute = "get" + attributeName.toFirstUpper» 
            «var setAttribute = "set" + attributeName.toFirstUpper» 
			/*
			 * «getAttribute»
			 */

			void «asyncClassName»::«getAttribute»(QSharedPointer<joynr::Future< «attributeType» > > future, QSharedPointer< joynr::ICallback< «attributeType» > > callback)
			{
			    assert (!future.isNull());
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «getAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«getAttribute»(future, callback);
			    }
			}
			
			void «asyncClassName»::«getAttribute»(QSharedPointer<joynr::Future< «attributeType» > > future)
			{
			    assert (!future.isNull());
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «getAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«getAttribute»(future);
			    }
			}
			
			void «asyncClassName»::«getAttribute»(QSharedPointer<joynr::ICallback< «attributeType» > > callback)
			{
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «getAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«getAttribute»(callback);
			    }
			}
			
			/*
			 * «setAttribute»
			 */
			
			void «asyncClassName»::«setAttribute»(QSharedPointer<joynr::Future<void> > future, QSharedPointer< joynr::ICallback<void> > callback, «attributeType» attributeValue) {
			    assert (!future.isNull());
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «setAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«setAttribute»(future, callback, attributeValue);
			    }
			}
			
			void «asyncClassName»::«setAttribute»(QSharedPointer<joynr::Future<void> > future, «attributeType» attributeValue) {
			    assert (!future.isNull());
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «setAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«setAttribute»(future, attributeValue);
			    }
			}
			
			void «asyncClassName»::«setAttribute»(QSharedPointer< joynr::ICallback<void> > callBack, «attributeType» attributeValue) {
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «setAttribute», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«setAttribute»(callBack, attributeValue);
			    }
			}

		«ENDFOR»
		«FOR method: getMethods(fInterface)»
			«var methodName = method.joynrName»
			«var outType = getMappedOutputParameter(method).head»
			«var paramsSignature = prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			«var params = prependCommaIfNotEmpty(getCommaSeperatedUntypedParameterList(method))»
			
			/*
			 * «methodName»
			 */
			
			void «asyncClassName»::«methodName»(QSharedPointer<joynr::Future<«outType»> > future, QSharedPointer< joynr::ICallback<«outType»> > callback «paramsSignature»)
			{
			    assert (!future.isNull());
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «methodName», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«methodName»(future, callback «params»);
			    }
			}
			
			void «asyncClassName»::«methodName»(QSharedPointer<joynr::Future<«outType»> > future «paramsSignature») {
			    assert (!future.isNull());
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «methodName», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«methodName»(future «params»);
			    }
			}
			
			void «asyncClassName»::«methodName»(QSharedPointer<joynr::ICallback<«outType»> > callback «paramsSignature») { 
			    if (connector==NULL){
			        LOG_WARN(logger, "proxy cannot invoke «methodName», because the communication end partner is not (yet) known");
			    }
			    else{
			        connector->«methodName»(callback «params»);
			    }
			}
			
		«ENDFOR»
		«getNamespaceEnder(fInterface)»
		'''	
	}	
	
			
}
