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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.NamingUtil

class InterfaceRequestInterpreterHTemplate extends InterfaceTemplate {

	@Inject extension TemplateBase
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«val headerGuard = ("GENERATED_INTERFACE_"+getPackagePathWithJoynrPrefix(francaIntf, "_", generateVersion)+
	"_"+interfaceName+"RequestInterpreter_h").toUpperCase»
«warning()»

#ifndef «headerGuard»
#define «headerGuard»

#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/Logger.h"

namespace joynr
{

class RequestCaller;
class Request;
class OneWayRequest;
class BaseReply;

namespace exceptions
{
class JoynrException;
} // namespace exceptions

} // namespace joynr

«getNamespaceStarter(francaIntf, generateVersion)»

/** @brief RequestInterpreter class for interface «interfaceName» */
class «interfaceName»RequestInterpreter: public joynr::IRequestInterpreter {
public:
	/** @brief Default constructor */
	«interfaceName»RequestInterpreter() = default;

	/** @brief Destructor */
	~«interfaceName»RequestInterpreter() override = default;

	/**
	 * @brief Implements IRequestInterpreter.execute().
	 * Executes method methodName with given parameters on the requestCaller object.
	 * @param requestCaller Object on which the method is to be executed
	 * @param request Request which was received
	 * @param onSuccess A callback function to be called once the asynchronous computation has
	 * finished with success. Its signature expects a BaseReply containing the output parameters.
	 * @param onError A callback function to be called once the asynchronous computation fails.
	 * Its signature expects a JoynrException.
	 */
	void execute(std::shared_ptr<joynr::RequestCaller> requestCaller,
				 Request& request,
				 std::function<void (BaseReply&& reply)>&& onSuccess,
				 std::function<void (const std::shared_ptr<exceptions::JoynrException>& exception)>&& onError) override;

	/**
	 * @brief Implements IRequestInterpreter.execute().
	 * Executes fire-and-forget method methodName with given parameters on the requestCaller object.
	 * @param request OneWayRequest which was received
	 */
	void execute(std::shared_ptr<joynr::RequestCaller> requestCaller,
					 OneWayRequest& request) override;

private:
	DISALLOW_COPY_AND_ASSIGN(«interfaceName»RequestInterpreter);
	ADD_LOGGER(«interfaceName»RequestInterpreter)
};

«getNamespaceEnder(francaIntf, generateVersion)»
#endif // «headerGuard»
'''
}
