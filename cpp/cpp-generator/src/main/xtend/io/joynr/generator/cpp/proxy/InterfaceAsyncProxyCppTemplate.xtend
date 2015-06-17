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
import org.franca.core.franca.FInterface
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate

class InterfaceAsyncProxyCppTemplate implements InterfaceTemplate{
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FInterface fInterface)
'''
«val interfaceName =  fInterface.joynrName»
«val className = interfaceName + "Proxy"»
«val asyncClassName = interfaceName + "AsyncProxy"»
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
		QSharedPointer<joynr::system::Address> messagingAddress,
		joynr::ConnectorFactory* connectorFactory,
		joynr::IClientCache *cache,
		const QString &domain,
		const joynr::MessagingQos &qosSettings,
		bool cached
) :
	joynr::ProxyBase(connectorFactory, cache, domain, getInterfaceName(), qosSettings, cached),
	«className»Base(messagingAddress, connectorFactory, cache, domain, qosSettings, cached)
{
}

«FOR attribute: getAttributes(fInterface)»
	«var attributeName = attribute.joynrName»
	«var attributeType = getMappedDatatypeOrList(attribute)»
	«IF attribute.readable»
		«var getAttribute = "get" + attributeName.toFirstUpper»
		/*
		 * «getAttribute»
		 */

		QSharedPointer<joynr::Future<«attributeType»>> «asyncClassName»::«getAttribute»(
				std::function<void(const joynr::RequestStatus& status, const «attributeType»& «attributeName»)> callbackFct)
		{
			if (connector==NULL){
				LOG_WARN(logger, "proxy cannot invoke «getAttribute», because the communication end partner is not (yet) known");
				//TODO error reaction for this case?
				QSharedPointer<joynr::Future<«attributeType»> > future;
				return future;
			}
			else{
				return connector->«getAttribute»(callbackFct);
			}
		}

	«ENDIF»
	«IF attribute.writable»
		«var setAttribute = "set" + attributeName.toFirstUpper»
		/*
		 * «setAttribute»
		 */

		QSharedPointer<joynr::Future<void>> «asyncClassName»::«setAttribute»(
				«attributeType» «attributeName»,
				std::function<void(const joynr::RequestStatus& status)> callbackFct)
		{
			if (connector==NULL){
				LOG_WARN(logger, "proxy cannot invoke «setAttribute», because the communication end partner is not (yet) known");
				//TODO error reaction for this case?
				QSharedPointer<joynr::Future<void> > future;
				return future;
			}
			else{
				return connector->«setAttribute»(«attributeName», callbackFct);
			}
		}

	«ENDIF»
«ENDFOR»
«FOR method: getMethods(fInterface)»
	«var methodName = method.joynrName»
	«var outputParameter = getMappedOutputParameter(method)»
	«var outputTypedParamList = prependCommaIfNotEmpty(getCommaSeperatedConstTypedOutputParameterList(method))»
	«var inputParamList = getCommaSeperatedUntypedParameterList(method)»
	/*
	 * «methodName»
	 */
	QSharedPointer<joynr::Future<«outputParameter.head»> > «asyncClassName»::«methodName»(
			«IF !method.inputParameters.empty»«getCommaSeperatedTypedInputParameterList(method)»,«ENDIF»
			std::function<void(const joynr::RequestStatus& status«outputTypedParamList»)> callbackFct)
	{
		if (connector==NULL){
			LOG_WARN(logger, "proxy cannot invoke «methodName», because the communication end partner is not (yet) known");
			//TODO error reaction for this case?
			QSharedPointer<joynr::Future<«outputParameter.head»> > future;
			return future;
		}
		else{
			return connector->«methodName»(«inputParamList»«IF !method.inputParameters.empty», «ENDIF»callbackFct);
		}
	}

«ENDFOR»
«getNamespaceEnder(fInterface)»
'''
}
