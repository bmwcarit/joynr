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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import org.franca.core.franca.FArgument
import io.joynr.generator.util.BroadcastTemplate
import io.joynr.generator.cpp.util.QtTypeUtil

class FilterTemplate implements BroadcastTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension QtTypeUtil

	def getCommaSeperatedEventArgumentListFromQList(Iterable<FArgument> arguments) {
		val returnStringBuilder = new StringBuilder();
		var i = 0
		for(FArgument argument : arguments){
			returnStringBuilder.append("eventValues[");
			returnStringBuilder.append(i++);
			returnStringBuilder.append("].value<");
			returnStringBuilder.append(argument.typeName);
			returnStringBuilder.append(">(),\n");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		}
		else{
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

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

«getIncludesFor(getAllPrimitiveTypes(serviceInterface))»

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/I«serviceInterface.name».h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«className»Parameters.h"
#include "joynr/IBroadcastFilter.h"
«getDllExportIncludeStatement()»

«getNamespaceStarter(serviceInterface)»
class «getDllExportMacro()» «className» : public IBroadcastFilter {
public:
	«className»() :
		IBroadcastFilter("«broadcastName»") { }

	~«className»() {}

	/*
	* Override this method to provide a filter logic implementation.
	*/
	virtual bool filter(
			«broadcast.commaSeperatedTypedConstOutputParameterList»,
			const «serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters) {
				return true;
	}
private:
	DISALLOW_COPY_AND_ASSIGN(«className»);

	virtual bool filter(
			const QList<QVariant>& eventValues,
			const BroadcastFilterParameters& filterParameters) {

		«serviceInterface.joynrName.toFirstUpper + broadcastName.toFirstUpper»BroadcastFilterParameters params;
		params.setFilterParameters(filterParameters.getFilterParameters());

		return filter(
				«getCommaSeperatedEventArgumentListFromQList(getOutputParameters(broadcast))»,
				params);
	}
};

«getNamespaceEnder(serviceInterface)»

#endif // «headerGuard»
'''
}