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
import java.util.Map
import org.franca.core.franca.FMethod

class InterfaceRequestCallerCppTemplate extends InterfaceTemplate {

	@Inject extension TemplateBase
	@Inject extension CppStdTypeUtil
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension InterfaceUtil
	@Inject extension MethodUtil

	override generate(boolean generateVersion)
'''
«var interfaceName = francaIntf.joynrName»
«warning()»
#include <functional>

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»RequestCaller.h"
«FOR datatype: getDataTypeIncludesFor(francaIntf, generateVersion)»
	#include «datatype»
«ENDFOR»
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/", generateVersion)»/«interfaceName»Provider.h"

«getNamespaceStarter(francaIntf, generateVersion)»
«interfaceName»RequestCaller::«interfaceName»RequestCaller(std::shared_ptr<«getPackagePathWithJoynrPrefix(francaIntf, "::", generateVersion)»::«interfaceName»Provider> _provider)
	: joynr::RequestCaller(«interfaceName»Provider::INTERFACE_NAME(), joynr::types::Version(_provider ? _provider->MAJOR_VERSION : 0, _provider ? _provider->MINOR_VERSION : 0)),
	  provider(std::move(_provider))
{
}

«IF !francaIntf.attributes.empty»
	// attributes
«ENDIF»
«FOR attribute : francaIntf.attributes»
	«var attributeName = attribute.joynrName»
	«val returnType = attribute.getTypeName(generateVersion)»
	«IF attribute.readable»
		void «interfaceName»RequestCaller::get«attributeName.toFirstUpper»(
				std::function<void(
						const «returnType»& «attributeName»
				)>&& onSuccess,
				std::function<void(
						const std::shared_ptr<exceptions::ProviderRuntimeException>&
				)> onError
		) {
			std::function<void(const exceptions::ProviderRuntimeException&)> onErrorWrapper =
			[onError] (const exceptions::ProviderRuntimeException& error) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
			};
			try {
				provider->get«attributeName.toFirstUpper»(std::move(onSuccess), std::move(onErrorWrapper));
			} catch (const exceptions::ProviderRuntimeException& e) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
			} catch (const exceptions::JoynrException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::get«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
			}
		}
	«ENDIF»
	«IF attribute.writable»
		void «interfaceName»RequestCaller::set«attributeName.toFirstUpper»(
				const «returnType»& «attributeName»,
				std::function<void()>&& onSuccess,
				std::function<void(
						const std::shared_ptr<exceptions::ProviderRuntimeException>&
				)> onError
		) {
			std::function<void(const exceptions::ProviderRuntimeException&)> onErrorWrapper =
			[onError] (const exceptions::ProviderRuntimeException& error) {
				onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
			};
			try {
				provider->set«attributeName.toFirstUpper»(«attributeName», std::move(onSuccess), std::move(onErrorWrapper));
			} catch (const exceptions::ProviderRuntimeException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::set«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
			} catch (const exceptions::JoynrException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::set«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
			}
		}
	«ENDIF»

«ENDFOR»
«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
«IF !francaIntf.methods.empty»
	// methods
«ENDIF»
«FOR method : francaIntf.methods»
	«val outputTypedParamList = method.getCommaSeperatedTypedConstOutputParameterList(generateVersion)»
	«val inputTypedParamList = method.getCommaSeperatedTypedConstInputParameterList(generateVersion)»
	«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
	«val methodName = method.joynrName»
	void «interfaceName»RequestCaller::«methodName»(
			«IF !method.inputParameters.empty»
				«inputTypedParamList»«IF !method.fireAndForget»,«ENDIF»
			«ENDIF»
			«IF !method.fireAndForget»
				«IF method.outputParameters.empty»
					std::function<void()>&& onSuccess,
				«ELSE»
					std::function<void(
							«outputTypedParamList»
					)>&& onSuccess,
				«ENDIF»
				std::function<void(
						const std::shared_ptr<exceptions::JoynrException>&
				)> onError
			«ENDIF»
	) {
		«IF !method.fireAndForget»
			«IF method.hasErrorEnum»
				«val errorTypeName = getErrorTypeName(method, methodToErrorEnumName, generateVersion)»
				std::function<void (const «errorTypeName»::«nestedEnumName»&)> onErrorWrapper =
						[onError] (const «errorTypeName»::«nestedEnumName»& errorEnum) {
							std::string typeName = «errorTypeName»::getTypeName();
							std::string name = «errorTypeName»::getLiteral(errorEnum);
							onError(std::make_shared<exceptions::ApplicationException>(typeName + "::" + name, std::make_shared<«errorTypeName»>(name)));
					};
			«ELSE»
			std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
					[onError] (const joynr::exceptions::ProviderRuntimeException& error) {
						onError(std::make_shared<exceptions::ProviderRuntimeException>(error));
					};
			«ENDIF»
		«ENDIF»
		try {
			provider->«methodName»(
					«IF !method.inputParameters.empty»«inputUntypedParamList»«IF !method.fireAndForget»,«ENDIF»«ENDIF»
					«IF !method.fireAndForget»
						std::move(onSuccess),
						std::move(onErrorWrapper)
					«ENDIF»
			);
		// ApplicationExceptions should not be created by the application itself to ensure
		// serializability. They are treated as JoynrExceptions. They can only be handled correctly
		// if the constructor is used properly (with the appropriate literal of the reported error
		// enumeration).
		} catch (const exceptions::ProviderRuntimeException& e) {
			«IF method.fireAndForget»
				std::string message = "Could not perform «interfaceName»RequestCaller::«methodName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				JOYNR_LOG_ERROR(logger(), message);
			«ELSE»
				onError(std::make_shared<exceptions::ProviderRuntimeException>(e));
			«ENDIF»
		} catch (const exceptions::JoynrException& e) {
			std::string message = "Could not perform «interfaceName»RequestCaller::«methodName.toFirstUpper», caught exception: " +
								e.getTypeName() + ":" + e.getMessage();
			«IF method.fireAndForget»
				JOYNR_LOG_ERROR(logger(), message);
			«ELSE»
				onError(std::make_shared<exceptions::ProviderRuntimeException>(message));
			«ENDIF»
		}
	}

«ENDFOR»

std::shared_ptr<IJoynrProvider> «interfaceName»RequestCaller::getProvider()
{
	return provider;
}

«getNamespaceEnder(francaIntf, generateVersion)»
'''

def getErrorTypeName(FMethod method, Map<FMethod, String> methodToErrorEnumName, boolean generateVersion) {
	if (method.errors !== null) {
		val packagePath = getPackagePathWithJoynrPrefix(method.errors, "::", generateVersion)
		packagePath + "::" + methodToErrorEnumName.get(method)
	} else{
		buildPackagePath(method.errorEnum, "::", true, generateVersion) + "::" + method.errorEnum.joynrName
	}
}
}
