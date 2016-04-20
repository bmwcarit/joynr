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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceAbstractProviderHTemplate extends InterfaceTemplate {
	@Inject private extension TemplateBase
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension CppStdTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension BroadcastUtil

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override generate()
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_")+
	"_"+interfaceName+"AbstractProvider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <string>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/AbstractJoynrProvider.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»Provider.h"

«FOR parameterType: getRequiredIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

«getDllExportIncludeStatement()»

«getNamespaceStarter(francaIntf)»

/** @brief Abstract provider class for interface «interfaceName» */
class «getDllExportMacro()» «interfaceName»AbstractProvider :
		public «getPackagePathWithJoynrPrefix(francaIntf, "::")»::«interfaceName»Provider,
		public joynr::AbstractJoynrProvider
{

public:
	/** @brief Default constructor */
	«interfaceName»AbstractProvider();

	/** @brief Destructor */
	~«interfaceName»AbstractProvider() override;

	/**
	 * @brief Get the interface name
	 * @return The name of the interface
	 */
	std::string getInterfaceName() const override;
«IF !francaIntf.attributes.isNullOrEmpty || !francaIntf.broadcasts.isNullOrEmpty»

	protected:
«ENDIF»
	«IF !francaIntf.attributes.isNullOrEmpty»

		// attributes
	«ENDIF»
	«FOR attribute : francaIntf.attributes»
		«IF attribute.notifiable»
			«var attributeName = attribute.joynrName»
			/**
			 * @brief «attributeName»Changed must be called by a concrete provider to signal attribute
			 * modifications. It is used to implement onchange subscriptions.
			 * @param «attributeName» the new attribute value
			 */
			void «attributeName»Changed(
					const «attribute.typeName»& «attributeName»
			) override;
		«ENDIF»
	«ENDFOR»
	«IF !francaIntf.broadcasts.isNullOrEmpty»

		// broadcasts
	«ENDIF»
	«FOR broadcast: francaIntf.broadcasts»
		«var broadcastName = broadcast.joynrName»
		/**
		 * @brief fire«broadcastName.toFirstUpper» must be called by a concrete provider to signal an occured
		 * event. It is used to implement broadcast publications.
		 «FOR parameter: getOutputParameters(broadcast)»
		 * @param «parameter.name» the value for the broadcast output parameter «parameter.name»
		 «ENDFOR»
		 */
		void fire«broadcastName.toFirstUpper»(
				«broadcast.commaSeperatedTypedConstOutputParameterList»
		) override;
	«ENDFOR»

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»AbstractProvider);
};
«getNamespaceEnder(francaIntf)»

#endif // «headerGuard»
'''
}
