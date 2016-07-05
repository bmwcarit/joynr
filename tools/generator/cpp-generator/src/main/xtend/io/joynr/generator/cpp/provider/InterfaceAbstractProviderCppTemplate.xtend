package io.joynr.generator.cpp.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.templates.util.InterfaceUtil

class InterfaceAbstractProviderCppTemplate extends InterfaceTemplate {

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension BroadcastUtil
	@Inject private extension InterfaceUtil

	override generate()
'''
«warning()»
«val interfaceName = francaIntf.joynrName»
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»AbstractProvider.h"
#include "joynr/InterfaceRegistrar.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestInterpreter.h"
#include "joynr/TypeUtil.h"

«FOR parameterType: getRequiredIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

«FOR broadcast: francaIntf.broadcasts.filter[selective]»
	«val broadcastName = broadcast.joynrName»
	«val broadCastFilterClassName = interfaceName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilter"»
	#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«broadCastFilterClassName».h"
«ENDFOR»

«getNamespaceStarter(francaIntf)»
«interfaceName»AbstractProvider::«interfaceName»AbstractProvider()
«IF hasSelectiveBroadcast(francaIntf)»
	:
	«FOR broadcast: francaIntf.broadcasts.filter[selective] SEPARATOR ','»
		«val broadcastName = broadcast.joynrName»
		«broadcastName»Filters()
	«ENDFOR»
«ENDIF»
{
	// Register a request interpreter to interpret requests to this interface
	joynr::InterfaceRegistrar::instance().registerRequestInterpreter<«interfaceName»RequestInterpreter>(getInterfaceName());
}

«interfaceName»AbstractProvider::~«interfaceName»AbstractProvider()
{
	// Unregister the request interpreter
	joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(getInterfaceName());
}

std::string «interfaceName»AbstractProvider::getInterfaceName() const {
	return I«interfaceName»Base::INTERFACE_NAME();
}

«FOR attribute : francaIntf.attributes»
	«IF attribute.notifiable»
		«var attributeType = attribute.type.resolveTypeDef»
		«var attributeName = attribute.joynrName»
		void «interfaceName»AbstractProvider::«attributeName»Changed(
				const «attribute.typeName»& «attributeName»
		) {
			onAttributeValueChanged("«attributeName»",«attributeName»);
		}
	«ENDIF»
«ENDFOR»

«FOR broadcast : francaIntf.broadcasts»
	«val broadcastName = broadcast.joynrName»
	void «interfaceName»AbstractProvider::fire«broadcastName.toFirstUpper»(
			«IF !broadcast.outputParameters.empty»
			«broadcast.commaSeperatedTypedConstOutputParameterList»
			«ENDIF»
	) {
		«IF broadcast.selective»
		fireSelectiveBroadcast
		«ELSE»
		fireBroadcast
		«ENDIF»
		("«broadcastName»"
		«IF broadcast.selective»
		, «broadcastName»Filters
		«ENDIF»
		«IF !broadcast.outputParameters.empty»
			,
			«FOR parameter : broadcast.outputParameters SEPARATOR ','»
				«parameter.joynrName»
			«ENDFOR»
		«ENDIF»
		);
	}

	«IF broadcast.selective»
		«val broadCastFilterClassName = interfaceName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilter"»
		void  «interfaceName»AbstractProvider::addBroadcastFilter(std::shared_ptr<«broadCastFilterClassName»> filter)
		{
			«broadcastName»Filters.push_back(std::move(filter));
		}
	«ENDIF»
«ENDFOR»
«getNamespaceEnder(francaIntf)»
'''
}