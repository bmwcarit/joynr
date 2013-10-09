package io.joynr.generator.cpp.communicationmodel
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
import org.franca.core.franca.FType
import io.joynr.generator.cpp.util.InterfaceUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions

class InterfaceHTemplate {
	
	@Inject
	private extension TemplateBase
	
	@Inject
	private extension InterfaceUtil
	
	@Inject
	private extension JoynrCppGeneratorExtensions

	def generate(FInterface serviceInterface){
		val interfaceName = serviceInterface.name.toFirstUpper
		val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+"_I"+interfaceName+"_h").toUpperCase
	'''
	«warning()»

	#ifndef «headerGuard»
	#define «headerGuard»
	
	«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
		«IF datatype instanceof FType»
			«IF isComplex(datatype as FType)»
				«getNamespaceStarter(datatype as FType)» class «(datatype as FType).name»; «getNamespaceEnder(datatype as FType)»
			«ELSE»
				#include "«getIncludeOf(datatype as FType)»"
			«ENDIF»
		«ENDIF»
	«ENDFOR»
	
	«getDllExportIncludeStatement()»

	#include <QString>
	#include <QSharedPointer>

	namespace joynr {	
		class RequestStatus;
		template <class T> class Future;		
		template <class T> class ICallback;		
	}

	«getNamespaceStarter(serviceInterface)»

	/**
	 * @brief Base interface.
	 */
	class «getDllExportMacro()» I«interfaceName»Base {
	public: 
		I«interfaceName»Base();
		virtual ~I«interfaceName»Base() { }
		
		// Visual C++ does not export static const variables from DLLs
		// This getter is used instead
		static const QString getInterfaceName();
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
			using I«interfaceName»Sync::get«attributeName»;
			using I«interfaceName»Async::get«attributeName»;
			using I«interfaceName»Sync::set«attributeName»;
			using I«interfaceName»Async::set«attributeName»;
	    «ENDFOR»
		«FOR methodName: getUniqueMethodNames(serviceInterface)»
			using I«interfaceName»Sync::«methodName»;
			using I«interfaceName»Async::«methodName»;
		«ENDFOR»
	};
	
	«getNamespaceEnder(serviceInterface)»
	#endif // «headerGuard»
	'''
	}
}