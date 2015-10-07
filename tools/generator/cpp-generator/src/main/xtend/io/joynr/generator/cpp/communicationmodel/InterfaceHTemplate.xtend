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
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType

class InterfaceHTemplate implements InterfaceTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension CppInterfaceUtil

	@Inject
	private extension InterfaceUtil

	@Inject
	private extension AttributeUtil

	@Inject
	private extension NamingUtil

	@Inject extension CppStdTypeUtil

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_I"+interfaceName+"_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
	«IF datatype instanceof FType»
		«IF isComplex(datatype)»
			«getNamespaceStarter(datatype, true)»
				class «(datatype).joynrName»;
			«getNamespaceEnder(datatype, true)»
		«ELSE »
			#include "«getIncludeOf(datatype)»"
		«ENDIF»
	«ENDIF»
«ENDFOR»

«FOR include: serviceInterface.allPrimitiveTypes.includesFor.addElements(includeForArray, includeForString)»
	#include «include»
«ENDFOR»

«getDllExportIncludeStatement()»

#include <memory>
#include <functional>

namespace joynr {
	class RequestStatus;
	template <class ... Ts> class Future;
}

«getNamespaceStarter(serviceInterface)»

/**
 * @brief Base interface.
 */
class «getDllExportMacro()» I«interfaceName»Base {
public:
	I«interfaceName»Base();
	virtual ~I«interfaceName»Base() { }

	static const std::string& INTERFACE_NAME();
};

/**
 * @brief This is the «interfaceName» synchronous interface.
 *
 */
class «getDllExportMacro()» I«interfaceName»Sync : virtual public I«interfaceName»Base {
public:
	virtual ~I«interfaceName»Sync(){ }
	«produceSyncGetters(serviceInterface,true)»
	«produceSyncSetters(serviceInterface,true)»
	«produceSyncMethods(serviceInterface,true)»
};

/**
 * @brief This is the «interfaceName» asynchronous interface.
 *
 */
class «getDllExportMacro()» I«interfaceName»Async : virtual public I«interfaceName»Base {
public:
	virtual ~I«interfaceName»Async(){ }
	«produceAsyncGetters(serviceInterface,true)»
	«produceAsyncSetters(serviceInterface,true)»
	«produceAsyncMethods(serviceInterface,true)»
};

class «getDllExportMacro()» I«interfaceName» : virtual public I«interfaceName»Sync, virtual public I«interfaceName»Async {
public:
	virtual ~I«interfaceName»(){ }
	«FOR attribute: getAttributes(serviceInterface)»
		«val attributeName = attribute.name.toFirstUpper»
		«IF attribute.readable»
			using I«interfaceName»Sync::get«attributeName»;
			using I«interfaceName»Async::get«attributeName»Async;
		«ENDIF»
		«IF attribute.writable»
			using I«interfaceName»Sync::set«attributeName»;
			using I«interfaceName»Async::set«attributeName»Async;
		«ENDIF»
	«ENDFOR»
	«FOR methodName: getUniqueMethodNames(serviceInterface)»
		using I«interfaceName»Sync::«methodName»;
		using I«interfaceName»Async::«methodName»Async;
	«ENDFOR»
};

«getNamespaceEnder(serviceInterface)»
#endif // «headerGuard»
'''
}