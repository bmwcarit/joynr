package io.joynr.generator.cpp.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

class InterfaceRequestCallerHTemplate extends InterfaceTemplate {

	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"RequestCaller_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <functional>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/RequestCaller.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/Version.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/I«interfaceName».h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion).addElements(includeForString)»
	#include «parameterType»
«ENDFOR»
#include "joynr/Logger.h"

namespace joynr
{
class UnicastBroadcastListener;
class SubscriptionAttributeListener;
} // namespace joynr

«getNamespaceStarter(francaIntf, generateVersion)»

class «interfaceName»Provider;

/** @brief RequestCaller for interface «interfaceName» */
class «interfaceName»RequestCaller : public joynr::RequestCaller {
public:
	/**
	 * @brief parameterized constructor
	 * @param provider The provider instance
	 */
	explicit «interfaceName»RequestCaller(std::shared_ptr<«interfaceName»Provider> _provider);

	/** @brief Destructor */
	~«interfaceName»RequestCaller() override = default;

	«IF !francaIntf.attributes.empty»
		// attributes
	«ENDIF»
	«FOR attribute : francaIntf.attributes»
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
							const «attribute.getTypeName(generateVersion)»&
					)>&& onSuccess,
					std::function<void(
							const std::shared_ptr<exceptions::ProviderRuntimeException>&
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
					const «attribute.getTypeName(generateVersion)»& «attributeName»,
					std::function<void()>&& onSuccess,
					std::function<void(
							const std::shared_ptr<exceptions::ProviderRuntimeException>&
					)> onError
			);
		«ENDIF»

	«ENDFOR»
	«IF !francaIntf.methods.empty»
		// methods
	«ENDIF»
	«FOR method : francaIntf.methods»
		«val outputTypedParamList = method.getCommaSeperatedTypedConstOutputParameterList(generateVersion)»
		«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method, generateVersion)»
		/**
		 * @brief Implementation of Franca method «method.joynrName»
		 «IF !method.inputParameters.empty»
		 «FOR iparam: method.inputParameters»
		 * @param «iparam.joynrName» Method input parameter «iparam.joynrName»
		 «ENDFOR»
		 «ENDIF»
		 «IF !method.fireAndForget»
		 * @param onSuccess A callback function to be called once the asynchronous computation has
		 * finished with success. It must expect the output parameter list, if parameters are present.
		 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
		 «ENDIF»
		 */
		virtual void «method.joynrName»(
				«IF !method.inputParameters.empty»
					«inputTypedParamList»«IF !method.fireAndForget»,«ENDIF»
				«ENDIF»
				«IF !method.fireAndForget»
					«IF method.outputParameters.empty»
						std::function<void()>&& onSuccess,
					«ELSE»
						std::function<void(
								«outputTypedParamList»
						)>&& onSuccess,
					«ENDIF»
					std::function<void(
							const std::shared_ptr<exceptions::JoynrException>&
					)> onError
				«ENDIF»
		);

	«ENDFOR»

protected:
	std::shared_ptr<IJoynrProvider> getProvider() override;

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»RequestCaller);
	std::shared_ptr<«getPackagePathWithJoynrPrefix(francaIntf, "::", generateVersion)»::«interfaceName»Provider> provider;
	ADD_LOGGER(«interfaceName»RequestCaller)
};

«getNamespaceEnder(francaIntf, generateVersion)»
#endif // «headerGuard»
'''
}
