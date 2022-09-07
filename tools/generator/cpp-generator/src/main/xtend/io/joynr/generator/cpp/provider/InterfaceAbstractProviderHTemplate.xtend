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
import io.joynr.generator.templates.util.NamingUtil

class InterfaceAbstractProviderHTemplate extends InterfaceTemplate {
	@Inject extension TemplateBase
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension CppStdTypeUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"AbstractProvider_h").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

#include <string>
#include <vector>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/AbstractJoynrProvider.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»Provider.h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion)»
	#include «parameterType»
«ENDFOR»


«getNamespaceStarter(francaIntf, generateVersion)»

«IF !francaIntf.broadcasts.filter[selective].isNullOrEmpty»
// forward declare broadcast filter classes
«ENDIF»
«FOR broadcast: francaIntf.broadcasts.filter[selective]»
	class «getBroadcastFilterClassName(broadcast)»;
«ENDFOR»

/** @brief Abstract provider class for interface «interfaceName» */
class «interfaceName»AbstractProvider :
		public «getPackagePathWithJoynrPrefix(francaIntf, "::", generateVersion)»::«interfaceName»Provider,
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
	const std::string& getInterfaceName() const override;
«IF !francaIntf.attributes.isNullOrEmpty || !francaIntf.broadcasts.isNullOrEmpty»

    «FOR broadcast: francaIntf.broadcasts»
        «val broadcastName = broadcast.joynrName»
        «IF broadcast.selective»
            «val broadCastFilterClassName = interfaceName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilter"»
            /**
             * @brief Adds a specific broadcastFilter
             * @param filter an shared_ptr instance of «broadCastFilterClassName»
             */
            void addBroadcastFilter(std::shared_ptr<«broadCastFilterClassName»> filter);
        «ENDIF»
    «ENDFOR»

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
					const «attribute.getTypeName(generateVersion)»& «attributeName»
			) override;
		«ENDIF»
	«ENDFOR»
	«IF !francaIntf.broadcasts.isNullOrEmpty»

		// broadcasts
	«ENDIF»
	«FOR broadcast: francaIntf.broadcasts»
		«val broadcastName = broadcast.joynrName»
		/**
		 * @brief fire«broadcastName.toFirstUpper» must be called by a concrete provider to signal an occured
		 * event. It is used to implement broadcast publications.
		 «FOR parameter: getOutputParameters(broadcast)»
		 * @param «parameter.name» the value for the broadcast output parameter «parameter.name»
		 «ENDFOR»
		 «IF !broadcast.selective»
		 * @param partitions optional list of partitions for this broadcast. Broadcast notifications
		 * are only sent to subscribers for these partitions.
		 «ENDIF»
		 */
		void fire«broadcastName.toFirstUpper»(
				«IF !broadcast.outputParameters.empty»
					«broadcast.getCommaSeperatedTypedConstOutputParameterList(generateVersion)»«IF !broadcast.selective»,«ENDIF»
				«ENDIF»
				«IF !broadcast.selective»
					const std::vector<std::string>& partitions = std::vector<std::string>()
				«ENDIF»
		) override;
	«ENDFOR»

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»AbstractProvider);

	«FOR broadcast: francaIntf.broadcasts.filter[selective]»
		«val broadcastName = broadcast.joynrName»
		std::vector<std::shared_ptr<«getBroadcastFilterClassName(broadcast)»>> «broadcastName»Filters;
	«ENDFOR»
};
«getNamespaceEnder(francaIntf, generateVersion)»

#endif // «headerGuard»
'''
}
