package io.joynr.generator.cpp.communicationmodel
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
import io.joynr.generator.util.InterfaceTemplate
import java.util.HashSet
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.QtTypeUtil

class InterfaceCppTemplate implements InterfaceTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension QtTypeUtil

	@Inject
	private extension TemplateBase

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«warning()»

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
#include "qjson/serializer.h"
#include "joynr/MetaTypeRegistrar.h"

«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

#include "joynr/Future.h"

«getNamespaceStarter(serviceInterface)»

I«interfaceName»Base::I«interfaceName»Base()
{
	«val typeObjs = getAllComplexAndEnumTypes(serviceInterface)»
	«var replyMetatypes = getReplyMetatypes(serviceInterface)»
	«var broadcastMetatypes = getBroadcastMetatypes(serviceInterface)»

	«IF !typeObjs.isEmpty() || !replyMetatypes.empty»
		joynr::MetaTypeRegistrar& registrar = joynr::MetaTypeRegistrar::instance();
	«ENDIF»
	«FOR typeobj : typeObjs»
		«val datatype = typeobj as FType»

		// Register metatype «datatype.typeName»
		«IF isEnum(datatype)»
		{
			qRegisterMetaType<«getEnumContainer(datatype)»>();
			int id = qRegisterMetaType<«datatype.typeName»>();
			registrar.registerEnumMetaType<«getEnumContainer(datatype)»>();
			QJson::Serializer::registerEnum(id, «getEnumContainer(datatype)»::staticMetaObject.enumerator(0));
		}
		«ELSE»
			qRegisterMetaType<«datatype.typeName»>("«datatype.typeName»");
			registrar.registerMetaType<«datatype.typeName»>();
		«ENDIF»
	«ENDFOR»

	«IF serviceInterface.broadcasts.size > 0»
		/*
		 * Broadcast output parameters are packed into a single publication message when the
		 * broadcast occurs. They are encapsulated in a map. Hence, a new composite data type is
		 * needed for all broadcasts. The map is serialised into the publication message. When
		 * deserialising on consumer side, the right publication interpreter is chosen by calculating
		 * the type id for the composite type.
		*/
	«ENDIF»

	«FOR metatype : replyMetatypes»
		registrar.registerReplyMetaType<«metatype»>();
	«ENDFOR»
	«FOR broadcast: broadcastMetatypes»
		registrar.registerBroadcastMetaType<«broadcast»>();
	«ENDFOR»
}

const std::string& I«interfaceName»Base::INTERFACE_NAME()
{
	static const std::string INTERFACE_NAME("«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName.toLowerCase»");
	return INTERFACE_NAME;
}

«getNamespaceEnder(serviceInterface)»
'''
def getReplyMetatypes(FInterface serviceInterface) {
	var replyMetatypes = new HashSet();
	for (method: serviceInterface.methods) {
		if (!method.outputParameters.empty) {
			replyMetatypes.add(method.commaSeparatedOutputParameterTypes)
		}
	}
	for (attribute: serviceInterface.attributes) {
		if (attribute.readable) {
			replyMetatypes.add(attribute.typeName);
		}
	}
	return replyMetatypes;
}
def getBroadcastMetatypes(FInterface serviceInterface) {
	var broadcastMetatypes = new HashSet();
	for (broadcast: serviceInterface.broadcasts) {
		if (!broadcast.outputParameters.empty) {
			broadcastMetatypes.add(broadcast.commaSeparatedOutputParameterTypes)
		}
	}
	return broadcastMetatypes;
}
}
