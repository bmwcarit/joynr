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
import io.joynr.generator.cpp.util.CppMigrateToStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FInterface

class InterfaceAbstractProviderHTemplate implements InterfaceTemplate {
	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppMigrateToStdTypeUtil

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"AbstractProvider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <string>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/AbstractJoynrProvider.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»Provider.h"

«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

«getDllExportIncludeStatement()»

«getNamespaceStarter(serviceInterface)»

class «getDllExportMacro()» «interfaceName»AbstractProvider :
		public «getPackagePathWithJoynrPrefix(serviceInterface, "::")»::«interfaceName»Provider,
		public joynr::AbstractJoynrProvider
{

public:
	«interfaceName»AbstractProvider();
	virtual ~«interfaceName»AbstractProvider();

	virtual std::string getInterfaceName() const;
«IF !serviceInterface.attributes.isNullOrEmpty || !serviceInterface.broadcasts.isNullOrEmpty»

	protected:
«ENDIF»
	«IF !serviceInterface.attributes.isNullOrEmpty»

		// attributes
	«ENDIF»
	«FOR attribute : serviceInterface.attributes»
		«var attributeName = attribute.joynrName»
		/**
		* @brief «attributeName»Changed must be called by a concrete provider to signal attribute
		* modifications. It is used to implement onchange subscriptions.
		* @param «attributeName» the new attribute value
		*/
		virtual void «attributeName»Changed(
				const «attribute.typeName»& «attributeName»
		);
	«ENDFOR»
	«IF !serviceInterface.broadcasts.isNullOrEmpty»

		// broadcasts
	«ENDIF»
	«FOR broadcast: serviceInterface.broadcasts»
		«var broadcastName = broadcast.joynrName»
		/**
		* @brief fire«broadcastName.toFirstUpper» must be called by a concrete provider to signal an occured
		* event. It is used to implement broadcast publications.
		* @param «broadcastName» the new broadcast value
		*/
		virtual void fire«broadcastName.toFirstUpper»(
				«broadcast.commaSeperatedTypedConstOutputParameterList.substring(1)»
		);
	«ENDFOR»

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»AbstractProvider);
};
«getNamespaceEnder(serviceInterface)»

#endif // «headerGuard»
'''
}
