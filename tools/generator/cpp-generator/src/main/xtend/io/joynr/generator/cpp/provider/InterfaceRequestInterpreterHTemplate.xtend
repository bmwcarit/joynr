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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceRequestInterpreterHTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(serviceInterface, "_")+
	"_"+interfaceName+"RequestInterpreter_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include "joynr/PrivateCopyAssign.h"
«getDllExportIncludeStatement()»
#include "joynr/IRequestInterpreter.h"

#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"

#include "joynr/Variant.h"
#include <memory>
#include <string>
#include <vector>

«getNamespaceStarter(serviceInterface)»

/** @brief RequestInterpreter class for interface «interfaceName» */
class «getDllExportMacro()» «interfaceName»RequestInterpreter: public joynr::IRequestInterpreter {
public:
	/** @brief Default constructor */
	«interfaceName»RequestInterpreter();

	/** @brief Destructor */
	~«interfaceName»RequestInterpreter() override = default;

	/**
	 * @brief Implements IRequestInterpreter.execute().
	 * Executes method methodName with given parameters on the requestCaller object.
	 * @param requestCaller Object on which the method is to be executed
	 * @param methodName The name of the method to be executed
	 * @param paramValues The list of parameter values
	 * @param paramTypes The list of parameter types
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. It must expect the method out parameters.
	 * @param onError A callback function to be called once the asynchronous computation fails. It must expect the exception.
	 */
	void execute(std::shared_ptr<joynr::RequestCaller> requestCaller,
					 const std::string& methodName,
					 const std::vector<Variant>& paramValues,
					 const std::vector<std::string>& paramTypes,
					 std::function<void (std::vector<Variant>&& outParams)> onSuccess,
					 std::function<void (const exceptions::JoynrException& exception)> onError) override;

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»RequestInterpreter);
	ADD_LOGGER(«interfaceName»RequestInterpreter);
};

«getNamespaceEnder(serviceInterface)»
#endif // «headerGuard»
'''
}
