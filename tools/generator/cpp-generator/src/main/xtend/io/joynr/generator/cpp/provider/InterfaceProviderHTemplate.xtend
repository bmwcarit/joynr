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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceProviderHTemplate implements InterfaceTemplate{
	@Inject private extension TemplateBase
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension InterfaceUtil
	@Inject private extension MethodUtil

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"Provider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <string>

#include "joynr/PrivateCopyAssign.h"

#include "joynr/IJoynrProvider.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«interfaceName».h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/RequestCallerFactory.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"

«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

#include <memory>
«getDllExportIncludeStatement()»

«getNamespaceStarter(serviceInterface)»

/** @brief Provider class for interface «interfaceName» */
class «getDllExportMacro()» «interfaceName»Provider : public virtual IJoynrProvider
{

public:
	/** @brief Default constructor */
	«interfaceName»Provider();

	//for each Attribute the provider needs setters, sync and async getters.
	//They have default implementation for pushing Providers and can be overwritten by pulling Providers.

	/** @brief Destructor */
	virtual ~«interfaceName»Provider();

	static const std::string& INTERFACE_NAME();

	«IF !serviceInterface.attributes.empty»
		// attributes
	«ENDIF»
	«FOR attribute : serviceInterface.attributes»
		«var attributeName = attribute.joynrName»
		«IF attribute.readable»
			/**
			 * @brief Gets «attributeName.toFirstUpper»
			 * @param onSucess A callback function to be called once the asynchronous computation has
			 * finished with success. It must expect a request status object as well as the attribute value.
			 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
			 * @return the value of the attribute «attributeName.toFirstUpper»
			 */
			virtual void get«attributeName.toFirstUpper»(
					std::function<void(
							const «attribute.typeName»&
					)> onSuccess,
					std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
			) = 0;
		«ENDIF»
		«IF attribute.writable»
			/**
			 * @brief Sets «attributeName.toFirstUpper»
			 * @param «attributeName» the new value of the attribute
			 * @param onSuccess A callback function to be called once the asynchronous computation has
			 * finished with success. It must expect a request status object.
			 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
			 */
			virtual void set«attributeName.toFirstUpper»(
					const «attribute.typeName»& «attributeName»,
					std::function<void()> onSuccess,
					std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
			) = 0;
		«ENDIF»
		«IF attribute.notifiable»
			/**
			 * @brief «attributeName»Changed must be called by a concrete provider
			 * to signal attribute modifications. It is used to implement onchange
			 * subscriptions.
			 * @param «attributeName» the new attribute value
			 */
			virtual void «attributeName»Changed(
					const «attribute.typeName»& «attributeName»
			) = 0;
		«ENDIF»

	«ENDFOR»
	«IF !serviceInterface.methods.empty»
		// methods
	«ENDIF»
	«val methodToErrorEnumName = serviceInterface.methodToErrorEnumName»
	«FOR method : serviceInterface.methods»
		«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
		«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method)»
		/**
		 * @brief Implementation of the Franca method «method.joynrName»
		 * @param onSuccess A callback function to be called once the asynchronous computation has
		 * finished with success. It must expect a request status object as well as the method out parameters.
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
				«IF method.hasErrorEnum»
					«IF method.errors != null»
						«val packagePath = getPackagePathWithJoynrPrefix(method.errors, "::")»
						std::function<void (const «packagePath»::«methodToErrorEnumName.get(method)»::«nestedEnumName»& errorEnum)> onError
					«ELSE»
						std::function<void (const «method.errorEnum.typeName»& errorEnum)> onError
					«ENDIF»
				«ELSE»
				std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
				«ENDIF»
		) = 0;

	«ENDFOR»
	«IF !serviceInterface.broadcasts.empty»
		// broadcasts
	«ENDIF»
	«FOR broadcast : serviceInterface.broadcasts»
		«var broadcastName = broadcast.joynrName»
		/**
		 * @brief fire«broadcastName.toFirstUpper» must be called by a concrete
		 * provider to signal an occured event. It is used to implement broadcast
		 * publications.
		 * @param «broadcastName» the new broadcast value
		 */
		virtual void fire«broadcastName.toFirstUpper»(
				«broadcast.commaSeperatedTypedConstOutputParameterList»
		) = 0;

	«ENDFOR»
private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»Provider);
};
«getNamespaceEnder(serviceInterface)»

namespace joynr {

// Helper class for use by the RequestCallerFactory.
// This class creates instances of «interfaceName»RequestCaller
template<>
class RequestCallerFactoryHelper<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> {
public:
	std::shared_ptr<joynr::RequestCaller> create(std::shared_ptr<«getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider> provider) {
		return std::shared_ptr<joynr::RequestCaller>(new «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»RequestCaller(provider));
	}
};
} // namespace joynr

#endif // «headerGuard»
'''
}
