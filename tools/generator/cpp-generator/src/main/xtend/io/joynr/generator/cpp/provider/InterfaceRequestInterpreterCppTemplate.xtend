package io.joynr.generator.cpp.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceRequestInterpreterCppTemplate extends InterfaceTemplate {

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension InterfaceUtil

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override generate()
'''
«val interfaceName = francaIntf.joynrName»
«warning()»
#include <functional>
#include <tuple>

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestInterpreter.h"

#include "joynr/Request.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestCaller.h"
#include "joynr/Util.h"
#include "joynr/TypeUtil.h"

«FOR parameterType: getRequiredIncludesFor(francaIntf)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(francaIntf)»

INIT_LOGGER(«interfaceName»RequestInterpreter);

«interfaceName»RequestInterpreter::«interfaceName»RequestInterpreter()
{
}

«val requestCallerName = interfaceName.toFirstLower+"RequestCallerVar"»
«val attributes = getAttributes(francaIntf)»
«val methods = getMethods(francaIntf)»
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
			«val attributeType = attribute.type.resolveTypeDef»
		«IF attribute.readable»
			if (methodName == "get«attributeName.toFirstUpper»"){
				std::function<void(«attribute.typeName» «attributeName»)> requestCallerOnSuccess =
						[onSuccess] («attribute.typeName» «attributeName») {
							std::vector<Variant> outParams;
								outParams.push_back(
									«IF attributeType.isEnum && attribute.isArray»
										joynr::TypeUtil::toVariant(util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(attributeType.derived)»>(«attribute.joynrName»))
									«ELSEIF attributeType.isEnum»
										Variant::make<«attribute.typeName»>(«attribute.joynrName»)
									«ELSEIF attribute.isArray»
										joynr::TypeUtil::toVariant<«attributeType.typeName»>(«attribute.joynrName»)
									«ELSEIF attributeType.isCompound»
										Variant::make<«attribute.typeName»>(«attribute.joynrName»)
									«ELSEIF attributeType.isMap»
										Variant::make<«attribute.typeName»>(«attribute.joynrName»)
									«ELSEIF attributeType.isByteBuffer»
										joynr::TypeUtil::toVariant(«attribute.joynrName»)
									«ELSE»
										Variant::make<«attribute.typeName»>(«attribute.joynrName»)
									«ENDIF»
							);
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
					«IF attributeType.isEnum && isArray(attribute)»
						«val attributeRef = joynrGenerationPrefix + "::util::convertVariantVectorToEnumVector<" + getTypeNameOfContainingClass(attributeType.derived) + ">(" + attributeName + "Var.get<std::vector<Variant>>())"»
						assert(«attributeName»Var.is<std::vector<Variant>>());
						«attribute.typeName» typedInput«attributeName.toFirstUpper» =
							«attributeRef»;
					«ELSEIF attributeType.isEnum»
						«attribute.typeName» typedInput«attributeName.toFirstUpper» =
							«joynrGenerationPrefix»::util::convertVariantToEnum<«getTypeNameOfContainingClass(attributeType.derived)»>(«attributeName»Var);
					«ELSEIF attribute.isArray»
						«val attributeRef = joynrGenerationPrefix + "::util::convertVariantVectorToVector<" + attributeType.typeName + ">(paramList)"»
						if (!«attributeName»Var.is<std::vector<Variant>>()) {
							onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
							return;
						}
						std::vector<Variant> paramList = «attributeName»Var.get<std::vector<Variant>>();
						«attribute.typeName» typedInput«attributeName.toFirstUpper» =
								«attributeRef»;
					«ELSEIF attributeType.isByteBuffer»
						//isArray
						if (!«attributeName»Var.is<std::vector<Variant>>()) {
							onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
							return;
						}
						std::vector<Variant> paramList = «attributeName»Var.get<std::vector<Variant>>();
						«attribute.typeName» typedInput«attributeName.toFirstUpper» =
								«joynrGenerationPrefix»::util::convertVariantVectorToVector<«byteBufferElementType»>(paramList);
					«ELSE»
						«var attributeRef = if (attributeType.float)
												"static_cast<float>(" + attributeName + "Var.get<double>())"
											else if (attributeType.string)
												"joynr::util::removeEscapeFromSpecialChars(" + attributeName + "Var.get<" + attribute.typeName + ">())"
											else
												attributeName + "Var.get<" + attribute.typeName + ">()"»
						«IF attribute.typeName.startsWith("std::int")»
							if (!(«attributeName»Var.is<std::uint64_t>() || «attributeName»Var.is<std::int64_t>())) {
						«ELSEIF attribute.typeName.startsWith("std::uint")»
							if (!«attributeName»Var.is<std::uint64_t>()) {
						«ELSEIF attributeType.float»
							if (!«attributeName»Var.is<double>()) {
						«ELSE»
							if (!«attributeName»Var.is<«attribute.typeName»>()) {
						«ENDIF»
							onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
							return;
						}
						«attribute.typeName» typedInput«attributeName.toFirstUpper» =
								«attributeRef»;
					«ENDIF»
					std::function<void()> requestCallerOnSuccess =
							[onSuccess] () {
								std::vector<Variant> outParams;
								onSuccess(std::move(outParams));
							};
					«requestCallerName»->set«attributeName.toFirstUpper»(typedInput«attributeName.toFirstUpper», requestCallerOnSuccess, onError);
			    } catch (const std::invalid_argument& exception) {
					onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper» («getJoynrTypeName(attribute)»)"));
			    }
				return;
			}
		«ENDIF»
		«ENDFOR»
	«ENDIF»
	«IF methods.size>0»
		«FOR method: getMethods(francaIntf)»
			«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
			«val methodName = method.joynrName»
			«val inputParams = getInputParameters(method)»
			«var iterator = -1»
			if (methodName == "«methodName»" && paramTypes.size() == «inputParams.size»
				«FOR input : inputParams»
					&& paramTypes.at(«iterator=iterator+1») == "«input.joynrTypeName»"
				«ENDFOR»
			) {
				«val outputTypedParamList = getCommaSeperatedTypedConstOutputParameterList(method)»
				std::function<void(«outputTypedParamList»)> requestCallerOnSuccess =
						[onSuccess](«outputTypedParamList»){
							std::vector<Variant> outParams;
							«FOR param : method.outputParameters»
								«val paramType = param.type.resolveTypeDef»
								outParams.push_back(
										«IF paramType.isEnum && param.isArray»
											joynr::TypeUtil::toVariant(util::convertEnumVectorToVariantVector<«getTypeNameOfContainingClass(paramType.derived)»>(«param.joynrName»))
										«ELSEIF paramType.isEnum»
											Variant::make<«getTypeName(param)»>(«param.joynrName»)
										«ELSEIF param.isArray»
											joynr::TypeUtil::toVariant<«paramType.typeName»>(«param.joynrName»)
										«ELSEIF paramType.isCompound»
											Variant::make<«getTypeName(param)»>(«param.joynrName»)
										«ELSEIF paramType.isMap»
											Variant::make<«getTypeName(param)»>(«param.joynrName»)
										«ELSEIF paramType.isByteBuffer»
											joynr::TypeUtil::toVariant(«param.joynrName»)
										«ELSE»
											Variant::make<«getTypeName(param)»>(«param.joynrName»)
										«ENDIF»
								);
							«ENDFOR»
							onSuccess(std::move(outParams));
						};


				«var iterator2 = -1»
				try {
					«FOR input : inputParams»
						«val inputName = input.joynrName»
						«val inputType = input.type.resolveTypeDef»
						Variant «inputName»Var(paramValues.at(«iterator2=iterator2+1»));
						«IF inputType.isEnum && input.isArray»
							//isEnumArray
							«input.typeName» «inputName» =
								«joynrGenerationPrefix»::util::convertVariantVectorToEnumVector<«getTypeNameOfContainingClass(inputType.derived)»> («inputName»Var.get<std::vector<Variant>>());
						«ELSEIF inputType.isEnum»
							//isEnum
							«input.typeName» «inputName» = «joynrGenerationPrefix»::util::convertVariantToEnum<«buildPackagePath(inputType.derived, "::", true) + "::" + inputType.joynrName»>(«inputName»Var);
						«ELSEIF input.isArray»
							//isArray
							if (!«inputName»Var.is<std::vector<Variant>>()) {
								onError(exceptions::MethodInvocationException("Illegal argument for method «methodName»: «inputName» («input.joynrTypeName»)"));
								return;
							}
							std::vector<Variant> «inputName»VarList = «inputName»Var.get<std::vector<Variant>>();
							std::vector<«inputType.typeName»> «inputName» = «joynrGenerationPrefix»::util::convertVariantVectorToVector<«inputType.typeName»>(«inputName»VarList);
						«ELSEIF inputType.isByteBuffer»
							//isArray
							if (!«inputName»Var.is<std::vector<Variant>>()) {
								onError(exceptions::MethodInvocationException("Illegal argument for method «methodName»: «inputName» («input.joynrTypeName»)"));
								return;
							}
							std::vector<Variant> «inputName»VarList = «inputName»Var.get<std::vector<Variant>>();
							std::vector<«byteBufferElementType»> «inputName» = «joynrGenerationPrefix»::util::convertVariantVectorToVector<«byteBufferElementType»>(«inputName»VarList);
						«ELSE»
							//«input.typeName»
							«var inputRef = if (inputType.float)
												"static_cast<float>(" + inputName + "Var.get<double>())"
											else if (inputType.string)
												"joynr::util::removeEscapeFromSpecialChars(" + inputName + "Var.get<" + input.typeName + ">())"
											else
												inputName + "Var.get<" + input.typeName + ">()"»
							«IF input.typeName.startsWith("std::int")»
								if (!(«inputName»Var.is<std::uint64_t>() || «inputName»Var.is<std::int64_t>())) {
							«ELSEIF input.typeName.startsWith("std::uint")»
								if (!«inputName»Var.is<std::uint64_t>()) {
							«ELSEIF inputType.float»
								if (!«inputName»Var.is<double>()) {
							«ELSE»
								if (!«inputName»Var.is<«input.typeName»>()) {
							«ENDIF»
								onError(exceptions::MethodInvocationException("Illegal argument for method «methodName»: «inputName» («input.joynrTypeName»)"));
								return;
							}
							«input.typeName» «inputName» = «inputRef»;
						«ENDIF»
					«ENDFOR»
					«requestCallerName»->«methodName»(
							«IF !method.inputParameters.empty»«inputUntypedParamList»,«ENDIF»
							requestCallerOnSuccess,
							onError);
				} catch (const std::invalid_argument& exception) {
					onError(exceptions::MethodInvocationException(exception.what()));
				}

				return;
			}
		«ENDFOR»
	«ENDIF»

	JOYNR_LOG_WARN(logger, "unknown method name for interface «interfaceName»: {}", methodName);
	onError(exceptions::MethodInvocationException("unknown method name for interface «interfaceName»: " + methodName));
}

«getNamespaceEnder(francaIntf)»
'''
}
