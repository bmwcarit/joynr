package io.joynr.generator.cpp.defaultProvider
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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil

class DefaultInterfaceProviderHTemplate extends InterfaceTemplate{

	@Inject extension TemplateBase

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject extension AttributeUtil

	@Inject extension MethodUtil

	@Inject extension InterfaceUtil

	@Inject extension JoynrCppGeneratorExtensions

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_Default"+interfaceName+"Provider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <functional>

«getDllExportIncludeStatement()»
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/I«interfaceName».h"
#include "joynr/Logger.h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion)»
	#include «parameterType»
«ENDFOR»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»AbstractProvider.h"

«getNamespaceStarter(francaIntf, generateVersion)»

class «getDllExportMacro()» Default«interfaceName»Provider : public «getPackagePathWithJoynrPrefix(francaIntf, "::", generateVersion)»::«interfaceName»AbstractProvider {

public:
	Default«interfaceName»Provider();

	~Default«interfaceName»Provider() override;

	«IF !francaIntf.attributes.empty»
		// attributes
	«ENDIF»
	«FOR attribute : francaIntf.attributes»
		«var attributeName = attribute.joynrName»
		«IF attribute.readable»
			void get«attributeName.toFirstUpper»(
					std::function<void(
							const «attribute.getTypeName(generateVersion)»&
					)> onSuccess,
					std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
			) override;
		«ENDIF»
		«IF attribute.writable»
			void set«attributeName.toFirstUpper»(
					const «attribute.getTypeName(generateVersion)»& _«attributeName»,
					std::function<void()> onSuccess,
					std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
			) override;
		«ENDIF»

	«ENDFOR»
	«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
	«IF !francaIntf.methods.empty»
		// methods
	«ENDIF»
	«FOR method : francaIntf.methods»
		«val outputTypedParamList = method.getCommaSeperatedTypedConstOutputParameterList(generateVersion)»
		«val inputTypedParamList = getCommaSeperatedTypedConstInputParameterList(method, generateVersion)»
		void «method.joynrName»(
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
							«val packagePath = getPackagePathWithJoynrPrefix(method.errors, "::", generateVersion)»
							std::function<void (const «packagePath»::«methodToErrorEnumName.get(method)»::«nestedEnumName»& errorEnum)> onError
						«ELSE»
							std::function<void (const «method.errorEnum.getTypeName(generateVersion)»& errorEnum)> onError
						«ENDIF»
					«ELSE»
					std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
					«ENDIF»
				«ENDIF»
		) override;

	«ENDFOR»
protected:
	«FOR attribute : getAttributes(francaIntf)»
		«attribute.getTypeName(generateVersion)» «attribute.joynrName»;
	«ENDFOR»

private:
	ADD_LOGGER(Default«interfaceName»Provider)

};

«getNamespaceEnder(francaIntf, generateVersion)»

#endif // «headerGuard»
'''
}
