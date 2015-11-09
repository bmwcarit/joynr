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
import io.joynr.generator.cpp.util.DatatypeSystemTransformation
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType

class InterfaceRequestInterpreterCppTemplate implements InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private QtTypeUtil qtTypeUtil
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

#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestInterpreter.h"

#include "joynr/Request.h"
#include "«getPackagePathWithJoynrPrefix(serviceInterface, "/")»/«interfaceName»RequestCaller.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/Util.h"
#include "joynr/TypeUtil.h"
#include "joynr/RequestStatus.h"
#include <cassert>

«FOR parameterType: qtTypeUtil.getRequiredIncludesFor(serviceInterface)»
	#include «parameterType»
«ENDFOR»

«getNamespaceStarter(serviceInterface)»

joynr::joynr_logging::Logger* «interfaceName»RequestInterpreter::logger = joynr::joynr_logging::Logging::getInstance()->getLogger("SDMO", "«interfaceName»RequestInterpreter");

«interfaceName»RequestInterpreter::«interfaceName»RequestInterpreter()
{
	«FOR datatype: getAllComplexAndEnumTypes(serviceInterface)»
		«IF datatype instanceof FType»
			«qtTypeUtil.registerMetatypeStatement(qtTypeUtil.getTypeName(datatype))»
		«ENDIF»
	«ENDFOR»
}

«val requestCallerName = interfaceName.toFirstLower+"RequestCallerVar"»
«val attributes = getAttributes(serviceInterface)»
«val methods = getMethods(serviceInterface)»
void «interfaceName»RequestInterpreter::execute(
		std::shared_ptr<joynr::RequestCaller> requestCaller,
		const std::string& methodName,
		const std::vector<Variant>& paramValues,
		const std::vector<std::string>& paramTypes,
		std::function<void (const QList<QVariant>&)> onSuccess,
		std::function<void (const exceptions::JoynrException& exception)> onError
) {
	Q_UNUSED(paramValues);//if all methods of the interface are empty, the paramValues would not be used and give a warning.
	Q_UNUSED(paramTypes);//if all methods of the interface are empty, the paramTypes would not be used and give a warning.
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
							«val convertedAttribute = qtTypeUtil.fromStdTypeToQTType(attribute, attributeName, true)»
							QVariant singleOutParam(«IF isArray(attribute)»joynr::Util::convertListToVariantList<«qtTypeUtil.getTypeName(attribute.type)»>(«convertedAttribute»)«ELSE»QVariant::fromValue(«convertedAttribute»)«ENDIF»);
							QList<QVariant> outParams;
							outParams.insert(0, singleOutParam);
							onSuccess(outParams);
						};
				«requestCallerName»->get«attributeName.toFirstUpper»(requestCallerOnSuccess, onError);
				return;
			}
		«ENDIF»
		«IF attribute.writable»
			if (methodName == "set«attributeName.toFirstUpper»" && paramTypes.size() == 1){
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
						onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper»"));
						return;
					}
					std::vector<Variant> paramList = «attributeName»Var.get<std::vector<Variant>>();
					«getTypeName(attribute)» typedInput«attributeName.toFirstUpper» = 
							«attributeRef»;
				«ELSE»
					«val attributeRef = attributeName + "Var.get<" + getTypeName(attribute) + ">()"»
					if (!«attributeName»Var.is<«getTypeName(attribute)»>()) {
						onError(exceptions::MethodInvocationException("Illegal argument for attribute setter set«attributeName.toFirstUpper»"));
						return;
					}
					«getTypeName(attribute)» typedInput«attributeName.toFirstUpper» =
							«attributeRef»;
				«ENDIF»
				std::function<void()> requestCallerOnSuccess =
						[onSuccess] () {
							QList<QVariant> outParams;
							onSuccess(outParams);
						};
				«requestCallerName»->set«attributeName.toFirstUpper»(typedInput«attributeName.toFirstUpper», requestCallerOnSuccess, onError);
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
							QList<QVariant> outParams;
							«var index = 0»
							«FOR param : method.outputParameters»
								«val convertedParameter = qtTypeUtil.fromStdTypeToQTType(param, param.joynrName, true)»
								outParams.insert(
										«index++»,
										«IF isArray(param)»
											joynr::Util::convertListToVariantList<«qtTypeUtil.getTypeName(param.type)»>(«convertedParameter»)
										«ELSE»
											QVariant::fromValue(«convertedParameter»)
										«ENDIF»
								);
							«ENDFOR»
							onSuccess(outParams);
						};


				«var iterator2 = -1»
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
						if (!«inputName»Var.is<«getTypeName(input)»>()) {
							onError(exceptions::MethodInvocationException("Illegal argument for method «methodName»: «inputName» («getJoynrTypeName(input)»)"));
							return;
						}
						«getTypeName(input)» «inputName» = «inputName»Var.get<«getTypeName(input)»>();
					«ENDIF»
				«ENDFOR»

				«requestCallerName»->«methodName»(
						«IF !method.inputParameters.empty»«inputUntypedParamList»,«ENDIF»
						requestCallerOnSuccess,
						onError);
				return;
			}
		«ENDFOR»
	«ENDIF»

	LOG_FATAL(logger, "unknown method name for interface «interfaceName»: " + TypeUtil::toQt(methodName));
	assert(false);
	onError(exceptions::MethodInvocationException("unknown method name for interface «interfaceName»: " + methodName));
}

«getNamespaceEnder(serviceInterface)»
'''
}
