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
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil

class InterfaceProviderHTemplate extends InterfaceTemplate {
	@Inject extension TemplateBase
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension CppStdTypeUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension BroadcastUtil
	@Inject extension InterfaceUtil
	@Inject extension MethodUtil

	override generate()
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_")+
	"_"+interfaceName+"Provider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <string>
«IF !francaIntf.broadcasts.filter[selective].empty»
	#include <vector>
«ENDIF»

#include "joynr/PrivateCopyAssign.h"

#include "joynr/IJoynrProvider.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/I«interfaceName».h"
#include "joynr/RequestCallerFactory.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestCaller.h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

#include <memory>
«getDllExportIncludeStatement()»

«getNamespaceStarter(francaIntf)»

/**
 * @brief Provider class for interface «interfaceName»
 *
 * @version «majorVersion».«minorVersion»
 */
class «getDllExportMacro()» «interfaceName»Provider : public virtual IJoynrProvider
{

public:
	/** @brief Default constructor */
	«interfaceName»Provider();

	//for each Attribute the provider needs setters, sync and async getters.
	//They have default implementation for pushing Providers and can be overwritten by pulling Providers.

	/** @brief Destructor */
	~«interfaceName»Provider() override;

	static const std::string& INTERFACE_NAME();
	/**
	 * @brief MAJOR_VERSION The major version of this provider interface as specified in the
	 * Franca model.
	 */
	static const std::int32_t MAJOR_VERSION;
	/**
	 * @brief MINOR_VERSION The minor version of this provider interface as specified in the
	 * Franca model.
	 */
	static const std::int32_t MINOR_VERSION;

	«IF !francaIntf.attributes.empty»
		// attributes
	«ENDIF»
	«FOR attribute : francaIntf.attributes»
		«var attributeName = attribute.joynrName»
		«IF attribute.readable»
			/**
			 * @brief Gets «attributeName.toFirstUpper»
			 *
			 «printThreadingDocumentation()»
			 *
			 «printCallbackDocumentation("with the attribute value")»
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
			 *
			 «printThreadingDocumentation()»
			 *
			 * @param «attributeName» the new value of the attribute
			 «printCallbackDocumentation("")»
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
	«IF !francaIntf.methods.empty»
		// methods
	«ENDIF»
	«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
	«FOR method : francaIntf.methods»
		«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
		«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method)»
		/**
		 * @brief Implementation of the Franca method «method.joynrName»
		 *
		 «printThreadingDocumentation()»
		 *
		 «IF method.fireAndForget»
		 * This is a fire-and-forget method. Callers do not expect any response.
		 «ELSE»
		 «printCallbackDocumentation("")»
		 «ENDIF»
		 */
		virtual void «method.joynrName»(
				«IF !method.inputParameters.empty»
					«inputTypedParamList»«IF !method.fireAndForget»,«ENDIF»
				«ENDIF»
				«IF !method.fireAndForget»
					«IF method.outputParameters.empty»
						std::function<void()> onSuccess,
					«ELSE»
						std::function<void(
								«outputTypedParamList»
						)> onSuccess,
					«ENDIF»
					«IF method.hasErrorEnum»
						«IF method.errors !== null»
							«val packagePath = getPackagePathWithJoynrPrefix(method.errors, "::")»
							std::function<void (const «packagePath»::«methodToErrorEnumName.get(method)»::«nestedEnumName»& errorEnum)> onError
						«ELSE»
							std::function<void (const «method.errorEnum.typeName»& errorEnum)> onError
						«ENDIF»
					«ELSE»
						std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
					«ENDIF»
				«ENDIF»
		) = 0;

	«ENDFOR»
	«IF !francaIntf.broadcasts.empty»
		// broadcasts
	«ENDIF»
	«FOR broadcast : francaIntf.broadcasts»
		«var broadcastName = broadcast.joynrName»
		/**
		 * @brief fire«broadcastName.toFirstUpper» must be called by a concrete
		 * provider to signal an occured event. It is used to implement broadcast
		 * publications.
		 «FOR parameter: getOutputParameters(broadcast)»
		 * @param «parameter.name» the value for the broadcast output parameter «parameter.name»
		 «ENDFOR»
		 «IF !broadcast.selective»
		 * @param partitions optional list of partitions for this broadcast. Broadcast notifications
		 * are only sent to subscribers for these partitions.
		 «ENDIF»
		 */
		virtual void fire«broadcastName.toFirstUpper»(
				«IF !broadcast.outputParameters.empty»
					«broadcast.commaSeperatedTypedConstOutputParameterList»«IF !broadcast.selective»,«ENDIF»
				«ENDIF»
				«IF !broadcast.selective»
					const std::vector<std::string>& partitions = std::vector<std::string>()
				«ENDIF»
		) = 0;

	«ENDFOR»
private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»Provider);
};
«getNamespaceEnder(francaIntf)»

«var packagePrefix = getPackagePathWithJoynrPrefix(francaIntf, "::")»

namespace joynr {

// specialization of traits class RequestCallerTraits
// this links «interfaceName»Provider with «interfaceName»RequestCaller
template <>
struct RequestCallerTraits<«packagePrefix»::«interfaceName»Provider>
{
	using RequestCaller = «packagePrefix»::«interfaceName»RequestCaller;
};

} // namespace joynr

#endif // «headerGuard»
'''

def printThreadingDocumentation()
'''
* This method is called by a joynr middleware thread. The provider implementation
* must not block this thread; it must be released immediately after the method is
* called. Computations or further blocking calls must be performed asynchronously.
* Return the result of these computations by calling the onSuccess or onError
* callbacks asynchronously. N.B. Internal joynr data structures of joynr messages are
* captured in the callbacks and will be first released after the callback onSuccess
* or onError is called.
'''

def printCallbackDocumentation(String onSuccessParameter)
'''
* @param onSuccess A callback function to be called «onSuccessParameter» once the asynchronous computation has
* finished with success. It expects a request status object as parameter.
* @param onError A callback function to be called once the asynchronous computation fails. It expects an exception.
'''
}
