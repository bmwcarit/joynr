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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil

class InterfaceRequestInterpreterCppTemplate extends InterfaceTemplate {

	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil
	@Inject extension InterfaceUtil

	override generate(boolean generateVersion)
'''
«val interfaceName = francaIntf.joynrName»
«warning()»
#include <functional>
#include <tuple>

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»RequestInterpreter.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»RequestCaller.h"
#include "joynr/Util.h"
#include "joynr/Request.h"
#include "joynr/OneWayRequest.h"
#include "joynr/BaseReply.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"

«FOR parameterType: getDataTypeIncludesFor(francaIntf, generateVersion)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(francaIntf, generateVersion)»

«val requestCallerName = interfaceName.toFirstLower+"RequestCallerVar"»
«val attributes = getAttributes(francaIntf)»
«val methodsWithoutFireAndForget = getMethods(francaIntf).filter[!fireAndForget]»
void «interfaceName»RequestInterpreter::execute(
		std::shared_ptr<joynr::RequestCaller> requestCaller,
		Request& request,
		std::function<void (BaseReply&& reply)>&& onSuccess,
		std::function<void (const std::shared_ptr<exceptions::JoynrException>& exception)>&& onError
) {
	assert(requestCaller);
	if (!requestCaller) {
		JOYNR_LOG_FATAL(logger(), "{}: requestCaller is nullptr, aborting", "«interfaceName»RequestInterpreter::execute(...)");
		return;
	}
	assert(onSuccess);
	if (!onSuccess) {
		JOYNR_LOG_FATAL(logger(), "{}: onSuccess is nullptr, aborting", "«interfaceName»RequestInterpreter::execute(...)");
		return;
	}
	assert(onError);
	if (!onError) {
		JOYNR_LOG_FATAL(logger(), "{}: onError is nullptr, aborting", "«interfaceName»RequestInterpreter::execute(...)");
		return;
	}
	«IF francaIntf.hasReadAttribute || francaIntf.hasWriteAttribute || !methodsWithoutFireAndForget.empty»
		// cast generic RequestCaller to «interfaceName»Requestcaller
		std::shared_ptr<«interfaceName»RequestCaller> «requestCallerName» =
				std::dynamic_pointer_cast<«interfaceName»RequestCaller>(requestCaller);

		const std::vector<std::string>& paramTypes = request.getParamDatatypes();
		const std::string& methodName = request.getMethodName();

		// execute operation
		«IF !attributes.empty»
			«FOR attribute : attributes»
				«val attributeName = attribute.joynrName»
				«IF attribute.readable»
				if (methodName == "get«attributeName.toFirstUpper»" && paramTypes.size() == 0){
					try {
						auto requestCallerOnSuccess =
								[onSuccess = std::move(onSuccess)](«attribute.getTypeName(generateVersion)» «attributeName»){
									BaseReply reply;
									reply.setResponse(std::move(«attributeName»));
									onSuccess(std::move(reply));
								};
						«requestCallerName»->get«attributeName.toFirstUpper»(
																			std::move(requestCallerOnSuccess),
																			onError);
					} catch (const std::exception& exception) {
						const std::string errorMessage = "Unexpected exception occurred in attribute getter get«attributeName.toFirstUpper» (): " + std::string(exception.what());
						JOYNR_LOG_ERROR(logger(), errorMessage);
						onError(
							std::make_shared<exceptions::MethodInvocationException>(
								errorMessage,
								requestCaller->getProviderVersion()));
					}
					return;
				}
			«ENDIF»
			«IF attribute.writable»
				if (methodName == "set«attributeName.toFirstUpper»" && paramTypes.size() == 1){
					try {
						«attribute.getTypeName(generateVersion)» typedInput«attributeName.toFirstUpper»;
						request.getParams(typedInput«attributeName.toFirstUpper»);
						auto requestCallerOnSuccess =
								[onSuccess = std::move(onSuccess)] () {
									BaseReply reply;
									reply.setResponse();
									onSuccess(std::move(reply));
								};
						«requestCallerName»->set«attributeName.toFirstUpper»(
																			typedInput«attributeName.toFirstUpper»,
																			std::move(requestCallerOnSuccess),
																			onError);
					} catch (const std::exception& exception) {
						const std::string errorMessage = "Unexpected exception occurred in attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute, generateVersion)»): " + std::string(exception.what());
						JOYNR_LOG_ERROR(logger(), errorMessage);
						onError(
							std::make_shared<exceptions::MethodInvocationException>(
								errorMessage,
								requestCaller->getProviderVersion()));
					}
					return;
				}
			«ENDIF»
			«ENDFOR»
		«ENDIF»
		«FOR method: methodsWithoutFireAndForget»
			«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
			«val methodName = method.joynrName»
			«val inputParams = getInputParameters(method)»
			«var iterator = -1»
			if (methodName == "«methodName»" && paramTypes.size() == «inputParams.size»
				«FOR input : inputParams»
					&& paramTypes.at(«iterator=iterator+1») == "«input.getJoynrTypeName(generateVersion)»"
				«ENDFOR»
			) {
				«val outputTypedParamList = getCommaSeperatedTypedConstOutputParameterList(method, generateVersion)»
				auto requestCallerOnSuccess =
						[onSuccess = std::move(onSuccess)](«outputTypedParamList»){
							BaseReply reply;
							reply.setResponse(
							«FOR param : method.outputParameters SEPARATOR ','»
							«param.joynrName»
							«ENDFOR»
							);
							onSuccess(std::move(reply));
						};

				«FOR input : inputParams»
				«val inputName = input.joynrName»
				«val inputType = input.type.resolveTypeDef»
				«IF input.isArray»
				std::vector<«inputType.getTypeName(generateVersion)»> «inputName»;
				«ELSE»
				«inputType.getTypeName(generateVersion)» «inputName»;
				«ENDIF»
				«ENDFOR»
				try {
					«IF !method.inputParameters.empty»
					request.getParams(«inputUntypedParamList»);
					«ENDIF»
					«requestCallerName»->«methodName»(
							«IF !method.inputParameters.empty»«inputUntypedParamList»,«ENDIF»
							std::move(requestCallerOnSuccess),
							onError);
				} catch (const std::exception& exception) {
					const std::string errorMessage = "Unexpected exception occurred in method «methodName» (...): " + std::string(exception.what());
					JOYNR_LOG_ERROR(logger(), errorMessage);
					onError(std::make_shared<exceptions::MethodInvocationException>(errorMessage, requestCaller->getProviderVersion()));
				}

				return;
			}
		«ENDFOR»
	«ELSE»
		std::ignore = requestCaller;
		std::ignore = onSuccess;
	«ENDIF»

	JOYNR_LOG_WARN(logger(), "unknown method name for interface «interfaceName»: {}", request.getMethodName());
	onError(
		std::make_shared<exceptions::MethodInvocationException>(
			"unknown method name for interface «interfaceName»: " + request.getMethodName(),
			requestCaller->getProviderVersion()));
}

void «interfaceName»RequestInterpreter::execute(
		std::shared_ptr<joynr::RequestCaller> requestCaller,
		OneWayRequest& request
) {
	«val fireAndForgetMethods = getMethods(francaIntf).filter[fireAndForget]»
	«IF fireAndForgetMethods.empty»
		std::ignore = requestCaller;
	«ELSE»
		const std::vector<std::string>& paramTypes = request.getParamDatatypes();
		const std::string& methodName = request.getMethodName();
		// cast generic RequestCaller to «interfaceName»Requestcaller
		std::shared_ptr<«interfaceName»RequestCaller> «requestCallerName» =
				std::dynamic_pointer_cast<«interfaceName»RequestCaller>(requestCaller);

		// execute operation
		«FOR method : fireAndForgetMethods»
			«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
			«val methodName = method.joynrName»
			«val inputParams = getInputParameters(method)»
			«var iterator = -1»
			if (methodName == "«methodName»" && paramTypes.size() == «inputParams.size»
				«FOR input : inputParams»
					&& paramTypes.at(«iterator=iterator+1») == "«input.getJoynrTypeName(generateVersion)»"
				«ENDFOR»
			){
				«FOR input : inputParams»
				«val inputName = input.joynrName»
				«val inputType = input.type.resolveTypeDef»
				«IF input.isArray»
				std::vector<«inputType.getTypeName(generateVersion)»> «inputName»;
				«ELSE»
				«inputType.getTypeName(generateVersion)» «inputName»;
				«ENDIF»
				«ENDFOR»
				try {
					«IF !method.inputParameters.empty»
					request.getParams(«inputUntypedParamList»);
					«ENDIF»
					«requestCallerName»->«methodName»(«IF !method.inputParameters.empty»«inputUntypedParamList»«ENDIF»);
				} catch (const std::exception& exception) {
					const std::string errorMessage = "Unexpected exception occurred in method «methodName» (...): " + std::string(exception.what());
					JOYNR_LOG_ERROR(logger(), errorMessage);
				}
				return;
			}
		«ENDFOR»
	«ENDIF»

	JOYNR_LOG_WARN(logger(), "unknown method name for interface «interfaceName»: {}", request.getMethodName());
}
«getNamespaceEnder(francaIntf, generateVersion)»
'''
}
