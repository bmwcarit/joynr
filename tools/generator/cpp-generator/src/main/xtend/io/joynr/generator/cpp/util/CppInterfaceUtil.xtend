package io.joynr.generator.cpp.util
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
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FAttribute

class CppInterfaceUtil extends InterfaceUtil {
	@Inject extension NamingUtil
	@Inject extension CppStdTypeUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil
	@Inject extension JoynrCppGeneratorExtensions

	def printFutureReturnDefinition()
'''
	* @returns A future representing the result of the asynchronous method call. It provides methods
	* to wait for completion, to get the result or the request status object.
'''

	def printOnSuccessFctParamDefinition()
'''
	* @param onSuccess A callback function to be called once the asynchronous computation has
	* finished successfully. It must expect the method out parameters.
'''

	def printOnRuntimeErrorFctParamDefinition()
'''
	* @param onRuntimeError A callback function to be called once the asynchronous computation has
	* failed with an unexpected non-modeled exception. It must expect a JoynrRuntimeException object.
'''

	def printOnApplicationErrorFctParamDefinition()
'''
	* @param onApplicationError A callback function to be called once the asynchronous computation has
	* failed with an unexpected modeled exception. It must expect an Error enum as modeled in Franca.
'''

	def printMessagingQosParamDefinition()
'''
	* @param qos optional MessagingQos parameter; if specified, this will overwrite the MessagingQos that
	* was specified when building the proxy.
'''

	def produceSyncGetterSignature(FAttribute attribute, String className)
'''
	«val returnType = attribute.typeName»
	«val attributeName = attribute.joynrName»
	void «IF className !== null»«className»::«ENDIF»get«attributeName.toFirstUpper»(«returnType»& «attributeName», boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»)
'''

    def produceSyncGetterSignature(FAttribute attribute) {
    	return produceSyncGetterSignature(attribute, null);
    }

	def produceSyncGetterDeclarations(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.readable]»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Synchronous getter for the «attributeName» attribute.
		*
		* @param result The result that will be returned to the caller.
		«printMessagingQosParamDefinition»
		* @throws JoynrException if the request is not successful
		*/
		«IF pure»virtual «ENDIF»
		«produceSyncGetterSignature(attribute)»
		«IF pure»= 0«ELSE»override«ENDIF»;

	«ENDFOR»
'''

	def produceAsyncGetterSignature(FAttribute attribute, String className)
'''
	«val returnType = attribute.typeName»
	«val attributeName = attribute.joynrName»
	«val defaultArg = if(className === null) " = nullptr" else ""»
	std::shared_ptr<joynr::Future<«returnType»> > «IF className !== null»«className»::«ENDIF»get«attributeName.toFirstUpper»Async(
				std::function<void(const «returnType»& «attributeName»)> onSuccess«defaultArg»,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError«defaultArg»,
				boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»)
				noexcept
'''

	def produceAsyncGetterSignature(FAttribute attribute) {
    	return produceAsyncGetterSignature(attribute, null);
    }

	def produceAsyncGetterDeclarations(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.readable]»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Asynchronous getter for the «attributeName» attribute.
		*
		«printOnSuccessFctParamDefinition»
		«printOnRuntimeErrorFctParamDefinition»
		«printMessagingQosParamDefinition»
		«printFutureReturnDefinition»
		*/
		«IF pure»virtual «ENDIF»
		«produceAsyncGetterSignature(attribute)»
		«IF pure»= 0«ELSE»override«ENDIF»;
	«ENDFOR»
'''


	def produceSyncSetterSignature(FAttribute attribute, String className)
'''
	«val returnType = attribute.typeName»
	«val attributeName = attribute.joynrName»
	void «IF className !== null»«className»::«ENDIF»set«attributeName.toFirstUpper»(const «returnType»& «attributeName», boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»)
'''

	def produceSyncSetterSignature(FAttribute attribute) {
    	return produceSyncSetterSignature(attribute, null);
    }

	def produceSyncSetterDeclarations(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.writable]»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Synchronous setter for the «attributeName» attribute.
		*
		* @param «attributeName» The value to set.
		«printMessagingQosParamDefinition»
		* @throws JoynrException if the request is not successful
		*/
		«IF pure»virtual «ENDIF»
		«produceSyncSetterSignature(attribute)»
		«IF pure»= 0«ELSE»override«ENDIF»;
	«ENDFOR»
'''

	def produceAsyncSetterSignature(FAttribute attribute, String className)
'''
	«val returnType = attribute.typeName»
	«val attributeName = attribute.joynrName»
	«val defaultArg = if(className === null) " = nullptr" else ""»
	std::shared_ptr<joynr::Future<void> > «IF className !== null»«className»::«ENDIF»set«attributeName.toFirstUpper»Async(
				«returnType» «attributeName»,
				std::function<void(void)> onSuccess«defaultArg»,
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError«defaultArg»,
				boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»)
				noexcept
'''

    def produceAsyncSetterSignature(FAttribute attribute) {
    	return produceAsyncSetterSignature(attribute, null);
    }

	def produceAsyncSetterDeclarations(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.writable]»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Asynchronous setter for the «attributeName» attribute.
		*
		* @param «attributeName» The value to set.
		«printOnSuccessFctParamDefinition»
		«printOnRuntimeErrorFctParamDefinition»
		«printMessagingQosParamDefinition»
		«printFutureReturnDefinition»
		*/
		«IF pure»virtual «ENDIF»
		«produceAsyncSetterSignature(attribute)»
		«IF pure»= 0«ELSE»override«ENDIF»;
	«ENDFOR»
'''


	def produceSyncMethodSignature(FMethod method, String className)
'''
	«val outputTypedParamList = method.commaSeperatedTypedOutputParameterList»
	«val inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»
	void «IF className !== null»«className»::«ENDIF»«method.joynrName»(
				«IF !method.outputParameters.empty»
				«outputTypedParamList»,
				«ENDIF»
				«IF !method.inputParameters.empty»
				«inputTypedParamList»,
				«ENDIF»
				boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»
	)
'''

    def produceSyncMethodSignature(FMethod method) {
    	return produceSyncMethodSignature(method, null);
    }

	def produceSyncMethodDeclarations(FInterface serviceInterface, boolean pure)
'''
	«FOR method: getMethods(serviceInterface).filter[!fireAndForget]»
		/**
		* @brief Synchronous operation «method.joynrName».
		*
		«FOR outputParam: method.outputParameters»
		* @param «outputParam.typeName» «outputParam.joynrName» this is an output parameter
		*        and will be set within function «method.joynrName»
		«ENDFOR»
		«FOR inputParam: method.inputParameters»
		* @param «inputParam.typeName» «inputParam.joynrName»
		«ENDFOR»
		«printMessagingQosParamDefinition»
		* @throws JoynrException if the request is not successful
		*/
		«IF pure»virtual «ENDIF»
		«produceSyncMethodSignature(method)»
		«IF pure»= 0«ELSE»override«ENDIF»;
	«ENDFOR»
'''

	def getMethodErrorEnum(FInterface serviceInterface, FMethod method) {
    	val methodToErrorEnumName = serviceInterface.methodToErrorEnumName;
    	if(method.errors !== null) {
    		val packagePath = getPackagePathWithJoynrPrefix(method.errors, "::");
    		return packagePath + "::" + methodToErrorEnumName.get(method) + "::" + nestedEnumName;
    	}
    	else {
    		return method.errorEnum.typeName;
    	}
    }

	def produceAsyncMethodSignature(FInterface serviceInterface, FMethod method, String className)
'''
	«val outputParameters = method.commaSeparatedOutputParameterTypes»
	«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
	«val returnValue = "std::shared_ptr<joynr::Future<" + outputParameters + ">>"»
	«val defaultArg = if(className === null) " = nullptr" else ""»
	«returnValue» «IF className !== null»«className»::«ENDIF» «method.joynrName»Async(
				«IF !method.inputParameters.empty»
					«method.commaSeperatedTypedConstInputParameterList»,
				«ENDIF»
				std::function<void(«outputTypedParamList»)> onSuccess«defaultArg»,
				«IF method.hasErrorEnum»
					std::function<void (const «getMethodErrorEnum(serviceInterface, method)»& errorEnum)> onApplicationError«defaultArg»,
				«ENDIF»
				std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError«defaultArg»,
				boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»
	) noexcept
'''

	def produceAsyncMethodSignature(FInterface serviceInterface, FMethod method) {
    	return produceAsyncMethodSignature(serviceInterface, method, null);
    }


	def produceAsyncMethodDeclarations(FInterface serviceInterface, boolean pure, boolean useDefaultParam)
'''
	«FOR method: getMethods(serviceInterface).filter[!fireAndForget]»
		/**
		* @brief Asynchronous operation «method.joynrName».
		*
		«printOnSuccessFctParamDefinition»
		«IF method.hasErrorEnum»
			«printOnApplicationErrorFctParamDefinition»
		«ENDIF»
		«printOnRuntimeErrorFctParamDefinition»
		«printMessagingQosParamDefinition»
		«printFutureReturnDefinition»
		*/
		«IF pure»virtual «ENDIF»
		«produceAsyncMethodSignature(serviceInterface, method)»
		«IF pure»= 0«ELSE»override«ENDIF»;
	«ENDFOR»
'''

	def produceFireAndForgetMethodDeclarations(FInterface serviceInterface, boolean pure)
'''
	«FOR method: getMethods(serviceInterface).filter[fireAndForget]»
		/**
		* @brief FireAndForget operation «method.joynrName».
		*
		«FOR inputParam: method.inputParameters»
		* @param «inputParam.typeName» «inputParam.joynrName»
		«ENDFOR»
		«printMessagingQosParamDefinition»
		*/
		«IF pure»virtual «ENDIF»
		«produceFireAndForgetMethodSignature(method)»
		«IF pure»= 0«ELSE»override«ENDIF»;
	«ENDFOR»
'''

	def produceFireAndForgetMethodSignature(FMethod method, String className)
'''
	«val inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»
	void «IF className !== null»«className»::«ENDIF»«method.joynrName»(
	«IF !method.inputParameters.empty»
	«inputTypedParamList»,
	«ENDIF»
	boost::optional<joynr::MessagingQos> qos«IF className===null» = boost::none«ENDIF»)
'''

	def produceFireAndForgetMethodSignature(FMethod method) {
		return produceSyncMethodSignature(method, null);
	}

	def produceApplicationRuntimeErrorSplitForOnErrorWrapper(FInterface serviceInterface, FMethod method)
'''
	«IF method.hasErrorEnum»
		if (const exceptions::JoynrRuntimeException* runtimeError = dynamic_cast<const exceptions::JoynrRuntimeException*>(error.get())) {
			if(onRuntimeError) {
				onRuntimeError(*runtimeError);
			}
		}
		else if (const exceptions::ApplicationException* applicationError = dynamic_cast<const exceptions::ApplicationException*>(error.get())) {
			if(onApplicationError) {
				onApplicationError(muesli::EnumTraits<«getMethodErrorEnum(serviceInterface, method)»>::Wrapper::getEnum(applicationError->getName()));
			}
			else {
				const std::string errorMessage = "An ApplicationException type was received, but none was expected. Is the provider version incompatible with the consumer?";
				if (onRuntimeError) {
					onRuntimeError(exceptions::JoynrRuntimeException(errorMessage));
				}
				else {
					JOYNR_LOG_ERROR(logger(), errorMessage);
				}
			}
		}
		else {
			const std::string errorMessage = "Unknown exception: " + error->getTypeName() + ": " + error->getMessage();
			if (onRuntimeError) {
				onRuntimeError(exceptions::JoynrRuntimeException(errorMessage));
			}
			else {
				JOYNR_LOG_ERROR(logger(), errorMessage);
			}
		}
	«ELSE»
		if (onRuntimeError) {
			onRuntimeError(static_cast<const exceptions::JoynrRuntimeException&>(*error));
		}
	«ENDIF»
'''
}
