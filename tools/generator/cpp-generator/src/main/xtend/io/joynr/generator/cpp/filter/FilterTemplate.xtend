package io.joynr.generator.cpp.filter
/*
 * !!!
 *
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
import io.joynr.generator.templates.BroadcastTemplate
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface

class FilterTemplate implements BroadcastTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension NamingUtil
	@Inject extension BroadcastUtil

	def getCommaSeperatedEventArgumentListFromVariantList(Iterable<FArgument> arguments)'''
		«var i = 0»
		«FOR FArgument argument : arguments SEPARATOR ","»
			Util::valueOf<«argument.typeName»>(eventValues[«i++»])
		«ENDFOR»
''' 

	override generate(FInterface serviceInterface, FBroadcast broadcast)
'''
«val broadcastName =  broadcast.joynrName»
«val className = serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper + "BroadcastFilter"»
«val headerGuard = ("GENERATED_BROADCAST_FILTER_"+getPackagePathWithJoynrPrefix(broadcast, "_")+
	"_"+broadcastName+"_H").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
#include «parameterType»
«ENDFOR»

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«serviceInterface.name».h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«className»Parameters.h"
#include "joynr/IBroadcastFilter.h"
#include "joynr/TypeUtil.h"
«getDllExportIncludeStatement()»

«getNamespaceStarter(serviceInterface)»
/**
 * @brief Broadcast filter class for interface «serviceInterface.joynrName.toFirstUpper»,
 * broadcast «broadcastName»
 */
class «getDllExportMacro()» «className» : public IBroadcastFilter {
public:
	/** @brief Default constructor */
	«className»() :
			IBroadcastFilter("«broadcastName»")
	{
	}

	/** @brief Destructor */
	~«className»() override = default;

	/**
	 * @brief Filter method to decide whether a broadcast should be delivered.
	 * This method must be overriden to provider a filter logic implementation.
	 «FOR oparam: broadcast.outputParameters»
	 * @param «oparam.joynrName» Broadcast output parameter «oparam.joynrName» to be used for filtering
	 «ENDFOR»
	 * @param filterParameters The filter parameters
	 * @return true, if this broadcast should be published, false otherwise
	 */
	virtual bool filter(
			«broadcast.commaSeperatedTypedConstOutputParameterList.substring(1)»,
			const «serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters
	) = 0;
private:
	DISALLOW_COPY_AND_ASSIGN(«className»);

	bool filter (
			const std::vector<Variant>& eventValues,
			const BroadcastFilterParameters& filterParameters
	) override {
		«serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper»BroadcastFilterParameters params;
		params.setFilterParameters(filterParameters.getFilterParameters());

		return filter(
				«getCommaSeperatedEventArgumentListFromVariantList(getOutputParameters(broadcast))»,
				params
		);
	}
};

«getNamespaceEnder(serviceInterface)»

#endif // «headerGuard»
'''
}
