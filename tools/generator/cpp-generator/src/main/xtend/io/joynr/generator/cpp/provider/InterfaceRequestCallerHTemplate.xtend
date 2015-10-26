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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceRequestCallerHTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"RequestCaller_h").toUpperCase»
«warning()»
#include <functional>

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«getDllExportIncludeStatement()»
#include "joynr/RequestCaller.h"
#include "joynr/exceptions.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
#include <memory>

«FOR parameterType: getRequiredIncludesFor(serviceInterface).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(serviceInterface)»

class «interfaceName»Provider;

/** @brief RequestCaller for interface «interfaceName» */
class «getDllExportMacro()» «interfaceName»RequestCaller : public joynr::RequestCaller {
public:
	/**
	 * @brief parameterized constructor
	 * @param provider The provider instance
	 */
	explicit «interfaceName»RequestCaller(std::shared_ptr<«interfaceName»Provider> provider);

	/** @brief Destructor */
	virtual ~«interfaceName»RequestCaller(){}

	«IF !serviceInterface.attributes.empty»
		// attributes
	«ENDIF»
	«FOR attribute : serviceInterface.attributes»
		«var attributeName = attribute.joynrName»
		«IF attribute.readable»
			/**
			 * @brief Gets the value of the Franca attribute «attributeName.toFirstUpper»
			 * @param onSuccess A callback function to be called once the asynchronous computation has
			 * finished with success. It must expect a request status object as well as the return value.
			 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
			 */
			virtual void get«attributeName.toFirstUpper»(
					std::function<void(
							const «attribute.typeName»&
					)> onSuccess,
					std::function<void(
							const JoynrException&
					)> onError
			);
		«ENDIF»
		«IF attribute.writable»
			/**
			 * @brief Sets the value of the Franca attribute «attributeName.toFirstUpper»
			 * @param «attributeName» The new value of the attribute
			 * @param onSuccess A callback function to be called once the asynchronous computation has
			 * finished with success. It must expect a request status object.
			 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
			 */
			virtual void set«attributeName.toFirstUpper»(
					const «attribute.typeName»& «attributeName»,
					std::function<void()> onSuccess,
					std::function<void(
							const JoynrException&
					)> onError
			);
		«ENDIF»

	«ENDFOR»
	«IF !serviceInterface.methods.empty»
		// methods
	«ENDIF»
	«FOR method : serviceInterface.methods»
		«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
		«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method)»
		/**
		 * @brief Implementation of Franca method «method.joynrName»
		 «IF !method.inputParameters.empty»
		 «FOR iparam: method.inputParameters»
		 * @param «iparam.joynrName» Method input parameter «iparam.joynrName»
		 «ENDFOR»
		 «ENDIF»
		 * @param onSuccess A callback function to be called once the asynchronous computation has
		 * finished with success. It must expect the output parameter list, if parameters are present.
		 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
		 */
		virtual void «method.joynrName»(
				«IF !method.inputParameters.empty»
					«inputTypedParamList.substring(1)»,
				«ENDIF»
				«IF method.outputParameters.empty»
					std::function<void()> onSuccess,
				«ELSE»
					std::function<void(
							«outputTypedParamList.substring(1)»
					)> onSuccess,
				«ENDIF»
				std::function<void(
						const JoynrException&
				)> onError
		);

	«ENDFOR»
	/**
	 * @brief Register an attribute listener
	 * @param attributeName The name of the attribute for which a listener should be registered
	 * @param attributeListener The listener to be registered
	 */
	void registerAttributeListener(const std::string& attributeName, joynr::IAttributeListener* attributeListener);

	/**
	 * @brief Unregister an attribute listener
	 * @param attributeName The name of the attribute for which a listener should be unregistered
	 * @param attributeListener The listener to be unregistered
	 */
	void unregisterAttributeListener(const std::string& attributeName, joynr::IAttributeListener* attributeListener);

	/**
	 * @brief Register a broadcast listener
	 * @param broadcastName The name of the broadcast for which a listener should be registered
	 * @param broadcastListener The listener to be registered
	 */
	void registerBroadcastListener(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener);

	/**
	 * @brief Unregister a broadcast listener
	 * @param broadcastName The name of the broadcast for which a listener should be unregistered
	 * @param broadcastListener The listener to be unregistered
	 */
	void unregisterBroadcastListener(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener);

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»RequestCaller);
	std::shared_ptr<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider;
};

«getNamespaceEnder(serviceInterface)»
#endif // «headerGuard»
'''
}
