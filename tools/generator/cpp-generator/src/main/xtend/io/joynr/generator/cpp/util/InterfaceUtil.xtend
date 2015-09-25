package io.joynr.generator.cpp.util
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import org.franca.core.franca.FInterface

class InterfaceUtil {
	@Inject extension JoynrCppGeneratorExtensions
	@Inject extension CppStdTypeUtil

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

	def printOnErrorFctParamDefinition()
'''
	* @param onError A callback function to be called once the asynchronous computation has
	* failed. It must expect the request status object.
'''

	def produceSyncGetters(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.readable]»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Synchronous getter for the «attributeName» attribute.
		*
		* @param result The result that will be returned to the caller.
		* @returns the RequestStatus of the get«attributeName.toFirstUpper» call
		*/
		virtual joynr::RequestStatus get«attributeName.toFirstUpper»(
				«returnType»& result
		)«IF pure»=0«ENDIF»;

	«ENDFOR»
'''

	def produceAsyncGetters(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.readable]»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Asynchronous getter for the «attributeName» attribute.
		*
		«printOnSuccessFctParamDefinition»
		«printOnErrorFctParamDefinition»
		«printFutureReturnDefinition»
		*/
		virtual std::shared_ptr<joynr::Future<«returnType»> > get«attributeName.toFirstUpper»Async(
				std::function<void(const «returnType»& «attributeName.toFirstLower»)> onSuccess = nullptr,
				std::function<void(const joynr::RequestStatus& status)> onError = nullptr
		)«IF pure»=0«ENDIF»;
	«ENDFOR»
'''

	def produceSyncSetters(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.writable]»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Synchronous setter for the «attributeName» attribute.
		*
		* @param «attributeName.toFirstLower» The value to set.
		* @return the RequestStatus of the set«attributeName.toFirstUpper» call
		*/
		virtual joynr::RequestStatus set«attributeName.toFirstUpper»(
				const «returnType»& «attributeName.toFirstLower»
		)«IF pure»=0«ENDIF»;
	«ENDFOR»
'''

	def produceAsyncSetters(FInterface serviceInterface, boolean pure)
'''
	«FOR attribute: getAttributes(serviceInterface).filter[attribute | attribute.writable]»
		«val returnType = attribute.typeName»
		«val attributeName = attribute.joynrName»

		/**
		* @brief Asynchronous setter for the «attributeName» attribute.
		*
		* @param «attributeName.toFirstLower» The value to set.
		«printOnSuccessFctParamDefinition»
		«printOnErrorFctParamDefinition»
		«printFutureReturnDefinition»
		*/
		virtual std::shared_ptr<joynr::Future<void> > set«attributeName.toFirstUpper»Async(
				«returnType» «attributeName.toFirstLower»,
				std::function<void(void)> onSuccess = nullptr,
				std::function<void(const joynr::RequestStatus& status)> onError = nullptr
		)«IF pure»=0«ENDIF»;
	«ENDFOR»
'''

	def produceSyncMethods(FInterface serviceInterface, boolean pure)
'''
	«FOR method: getMethods(serviceInterface)»
		«val outputTypedParamList = method.commaSeperatedTypedOutputParameterList»
		«val inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»

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
		* @return the internal status of the request which will be returned to the caller.
		*/
		virtual joynr::RequestStatus «method.joynrName»(
				«outputTypedParamList»«IF method.outputParameters.size > 0 && method.inputParameters.size > 0», «ENDIF»«inputTypedParamList»
		)«IF pure»=0«ENDIF»;
	«ENDFOR»
'''

	def produceAsyncMethods(FInterface serviceInterface, boolean pure)
'''
	«FOR method: getMethods(serviceInterface)»
		«var outputParameters = method.commaSeparatedOutputParameterTypes»
		«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»

		/**
		* @brief Asynchronous operation «method.joynrName».
		*
		«printOnSuccessFctParamDefinition»
		«printOnErrorFctParamDefinition»
		«printFutureReturnDefinition»
		*/
		virtual std::shared_ptr<joynr::Future<«outputParameters»> > «method.joynrName»Async(
				«method.commaSeperatedTypedConstInputParameterList»«IF !method.inputParameters.empty»,«ENDIF»
				std::function<void(«outputTypedParamList»)> onSuccess = nullptr,
				std::function<void(const joynr::RequestStatus& status)> onError = nullptr
		)«IF pure»=0«ENDIF»;
	«ENDFOR»
'''
}
