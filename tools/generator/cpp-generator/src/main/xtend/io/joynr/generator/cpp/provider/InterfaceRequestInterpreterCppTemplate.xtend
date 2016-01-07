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
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceRequestInterpreterCppTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension InterfaceUtil

	override generate(FInterface serviceInterface)
'''
«val interfaceName = serviceInterface.joynrName»
«warning()»
#include <functional>
#include <tuple>

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"

#include "joynr/Request.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"
#include "joynr/Util.h"
#include "joynr/TypeUtil.h"
#include "joynr/RequestStatus.h"
#include <cassert>

«FOR parameterType: getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(serviceInterface)»

INIT_LOGGER(«interfaceName»RequestInterpreter);

«interfaceName»RequestInterpreter::«interfaceName»RequestInterpreter()
{
}

«val requestCallerName = interfaceName.toFirstLower+"RequestCallerVar"»
«val attributes = getAttributes(serviceInterface)»
«val methods = getMethods(serviceInterface)»
void «interfaceName»RequestInterpreter::execute(
		std::shared_ptr<joynr::RequestCaller> requestCaller,
		const std::string& methodName,
		const std::vector<Variant>& paramValues,
		const std::vector<std::string>& paramTypes,
		std::function<void (std::vector<Variant>&&)> onSuccess,
		std::function<void (const exceptions::JoynrException& exception)> onError
) {
	std::ignore = paramValues;//if all methods of the interface are empty, the paramValues would not be used and give a warning.
	std::ignore = paramTypes;//if all methods of the interface are empty, the paramTypes would not be used and give a warning.
	// cast generic RequestCaller to «interfaceName»Requestcaller
	std::shared_ptr<«interfaceName»RequestCaller> «requestCallerName» =
			std::dynamic_pointer_cast<«interfaceName»RequestCaller>(requestCaller);

	// execute operation
	«IF !attributes.empty»
		«FOR attribute : attributes»
			«val attributeName = attribute.joynrName»
			«val returnType = getTypeName(attribute)»
		«IF attribute.readable»
			if (methodName == "get«attributeName.toFirstUpper»"){
				std::function<void(«returnType» «attributeName»)> requestCallerOnSuccess =
						[onSuccess] («returnType» «attributeName») {
							Variant singleOutParam(«IF isArray(attribute)»joynr::TypeUtil::toVariant<«getTypeName(attribute.type)»>(«attributeName»)«ELSE»Variant::make<«getTypeName(attribute.type)»>(«attributeName»)«ENDIF»);
							std::vector<Variant> outParams;
							outParams.push_back(singleOutParam);
							onSuccess(std::move(outParams));
						};
				«requestCallerName»->get«attributeName.toFirstUpper»(requestCallerOnSuccess, onError);
				return;
			}
		«ENDIF»
		«IF attribute.writable»
			if (methodName == "set«attributeName.toFirstUpper»" && paramTypes.size() == 1){
				try {
					Variant «attributeName»Var(paramValues.at(0));
					«IF isEnum(attribute.type) && isArray(attribute)»
						«val attributeRef = joynrGenerationPrefix + "::Util::convertVariantVectorToEnumVector<" + getTypeNameOfContainingClass(attribute.type.derived) + ">(" + attributeName + "Var.get<std::vector<Variant>>())"»
						assert(«attributeName»Var.is<std::vector<Variant>>());
						«getTypeName(attribute)» typedInput«attributeName.toFirstUpper» =
							«attributeRef»;
					«ELSEIF isEnum(attribute.type)»
						«getTypeName(attribute)» typedInput«attributeName.toFirstUpper» =
							«joynrGenerationPrefix»::Util::convertVariantToEnum<«getTypeNameOfContainingClass(attribute.type.derived)»>(«attributeName»Var);
					«ELSEIF isArray(attribute)»
						«val attributeRef = joynrGenerationPrefix + "::Util::convertVariantVectorToVector<" + getTypeName(attribute.type) + ">(paramList)"»
						if (!«attributeName»Var.is<std::vector<Variant>>()) {
							onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
							return;
						}
						std::vector<Variant> paramList = «attributeName»Var.get<std::vector<Variant>>();
						«getTypeName(attribute)» typedInput«attributeName.toFirstUpper» = 
								«attributeRef»;
					«ELSE»
						«var attributeRef = if (attribute.type.float)
												"static_cast<float>(" + attributeName + "Var.get<double>())"
											else if (attribute.type.string)
												"joynr::removeEscapeFromSpecialChars(" + attributeName + "Var.get<" + getTypeName(attribute) + ">())"
											else
												attributeName + "Var.get<" + getTypeName(attribute) + ">()"»
						«IF getTypeName(attribute).startsWith("std::int")»
							if (!(«attributeName»Var.is<std::uint64_t>() || «attributeName»Var.is<std::int64_t>())) {
						«ELSEIF getTypeName(attribute).startsWith("std::uint")»
							if (!«attributeName»Var.is<std::uint64_t>()) {
						«ELSEIF attribute.type.float»
							if (!«attributeName»Var.is<double>()) {
						«ELSE»
							if (!«attributeName»Var.is<«getTypeName(attribute)»>()) {
						«ENDIF»
							onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
							return;
						}
						«getTypeName(attribute)» typedInput«attributeName.toFirstUpper» =
								«attributeRef»;
					«ENDIF»
					std::function<void()> requestCallerOnSuccess =
							[onSuccess] () {
								std::vector<Variant> outParams;
								onSuccess(std::move(outParams));
							};
					«requestCallerName»->set«attributeName.toFirstUpper»(typedInput«attributeName.toFirstUpper», requestCallerOnSuccess, onError);
			    } catch (std::invalid_argument exception) {
					onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
			    }
				return;
			}
		«ENDIF»
		«ENDFOR»
	«ENDIF»
	«IF methods.size>0»
		«FOR method: getMethods(serviceInterface)»
			«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
			«val methodName = method.joynrName»
			«val inputParams = getInputParameters(method)»
			«var iterator = -1»
			if (methodName == "«methodName»" && paramTypes.size() == «inputParams.size»
				«FOR input : inputParams»
					&& paramTypes.at(«iterator=iterator+1») == "«getJoynrTypeName(input)»"
				«ENDFOR»
			) {
				«val outputTypedParamList = getCommaSeperatedTypedConstOutputParameterList(method)»
				std::function<void(«outputTypedParamList»)> requestCallerOnSuccess =
						[onSuccess](«outputTypedParamList»){
							std::vector<Variant> outParams;
							«FOR param : method.outputParameters»
								outParams.push_back(
										«IF isArray(param)»
											joynr::TypeUtil::toVariant<«getTypeName(param.type)»>(«param.joynrName»)
										«ELSE»
											Variant::make<«getTypeName(param.type)»>(«param.joynrName»)
										«ENDIF»
								);
							«ENDFOR»
							onSuccess(std::move(outParams));
						};


				«var iterator2 = -1»
				try {
					«FOR input : inputParams»
						«val inputName = input.joynrName»
						Variant «inputName»Var(paramValues.at(«iterator2=iterator2+1»));
						«IF isEnum(input.type) && isArray(input)»
							//isEnumArray
							«getTypeName(input)» «inputName» =
								«joynrGenerationPrefix»::Util::convertVariantVectorToEnumVector<«getTypeNameOfContainingClass(input.type.derived)»> («inputName»Var.get<std::vector<Variant>>());
						«ELSEIF isEnum(input.type)»
							//isEnum
							«getTypeName(input)» «inputName» = «joynrGenerationPrefix»::Util::convertVariantToEnum<«buildPackagePath(input.type.derived, "::", true) + input.type.joynrName»>(«inputName»Var);
						«ELSEIF isArray(input)»
							//isArray
							if (!«inputName»Var.is<std::vector<Variant>>()) {
								onError(exceptions::MethodInvocationException("Illegal argument for method «methodName»: «inputName» («getJoynrTypeName(input)»)"));
								return;
							}
							std::vector<Variant> «inputName»VarList = «inputName»Var.get<std::vector<Variant>>();
							std::vector<«getTypeName(input.type)»> «inputName» = «joynrGenerationPrefix»::Util::convertVariantVectorToVector<«getTypeName(input.type)»>(«inputName»VarList);
						«ELSE»
							//«getTypeName(input)»
							«var inputRef = if (input.type.float)
												"static_cast<float>(" + inputName + "Var.get<double>())"
											else if (input.type.string)
												"joynr::removeEscapeFromSpecialChars(" + inputName + "Var.get<" + getTypeName(input) + ">())"
											else
												inputName + "Var.get<" + getTypeName(input) + ">()"»
							«IF getTypeName(input).startsWith("std::int")»
								if (!(«inputName»Var.is<std::uint64_t>() || «inputName»Var.is<std::int64_t>())) {
							«ELSEIF getTypeName(input).startsWith("std::uint")»
								if (!«inputName»Var.is<std::uint64_t>()) {
							«ELSEIF input.type.float»
								if (!«inputName»Var.is<double>()) {
							«ELSE»
								if (!«inputName»Var.is<«getTypeName(input)»>()) {
							«ENDIF»
								onError(exceptions::MethodInvocationException("Illegal argument for method «methodName»: «inputName» («getJoynrTypeName(input)»)"));
								return;
							}
							«getTypeName(input)» «inputName» = «inputRef»;
						«ENDIF»
					«ENDFOR»
					«requestCallerName»->«methodName»(
							«IF !method.inputParameters.empty»«inputUntypedParamList»,«ENDIF»
							requestCallerOnSuccess,
							onError);
				} catch (std::invalid_argument exception) {
					onError(exceptions::MethodInvocationException(exception.what()));
				}

				return;
			}
		«ENDFOR»
	«ENDIF»

	JOYNR_LOG_WARN(logger, "unknown method name for interface «interfaceName»: {}", methodName);
	onError(exceptions::MethodInvocationException("unknown method name for interface «interfaceName»: " + methodName));
}

«getNamespaceEnder(serviceInterface)»
'''
}
