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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType

class InterfaceProviderCppTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension QtTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension InterfaceUtil

	override generate(FInterface serviceInterface)
'''
«warning()»
«val interfaceName = serviceInterface.joynrName»
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"
#include "joynr/RequestStatus.h"
#include "joynr/TypeUtil.h"
«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(serviceInterface)»
«interfaceName»Provider::«interfaceName»Provider()
{
	// Register a request interpreter to interpret requests to this interface
	joynr::InterfaceRegistrar::instance().registerRequestInterpreter<«interfaceName»RequestInterpreter>(INTERFACE_NAME());

	«val typeObjs = getAllComplexAndEnumTypes(serviceInterface, true)»

	«IF !typeObjs.isEmpty()»
		joynr::MetaTypeRegistrar& registrar = joynr::MetaTypeRegistrar::instance();
	«ENDIF»
	«FOR typeobj : typeObjs»
		«val datatype = typeobj as FType»

		// Register metatype «datatype.typeName»
		«IF isEnum(datatype)»
		{
			«registerMetatypeStatement(datatype.typeNameOfContainingClass)»
			int id = «registerMetatypeStatement(datatype.typeName)»
			registrar.registerEnumMetaType<«datatype.typeNameOfContainingClass»>();
			QJson::Serializer::registerEnum(id, «datatype.typeNameOfContainingClass»::staticMetaObject.enumerator(0));
		}
		«ELSE»
			«registerMetatypeStatement(datatype.typeName)»
			registrar.registerMetaType<«datatype.typeName»>();
		«ENDIF»
	«ENDFOR»
}

«interfaceName»Provider::~«interfaceName»Provider()
{
	// Unregister the request interpreter
	joynr::InterfaceRegistrar::instance().unregisterRequestInterpreter(
			INTERFACE_NAME()
	);
}

const std::string& «interfaceName»Provider::INTERFACE_NAME()
{
	static const std::string INTERFACE_NAME("«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName»");
	return INTERFACE_NAME;
}

«getNamespaceEnder(serviceInterface)»
'''
}
