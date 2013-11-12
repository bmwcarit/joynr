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
	@Inject	extension JoynrCppGeneratorExtensions
	
	def printFutureParamDefinition()
	'''
		* @param future With this object, you will be able to query the joynrInternalStatus and result of
		* the request.
	'''

	def printCallbackParamDefinition()
	'''
		* @param callback A callback typed according to the expected return value type.
		* This class will be called, together with a return value when the request succeeds
		* or fails.
	'''
	
	def produceSyncGetters(FInterface serviceInterface, boolean pure)
	'''
		«FOR attribute: getAttributes(serviceInterface)»
			«val returnType = getMappedDatatypeOrList(attribute)»
			«val attributeName = attribute.joynrName»
			 
			/**
			* @brief Synchronous getter for the «attributeName» attribute.
			*
			* @param joynrInternalStatus The joynrInternalStatus of the request which will be returned to the caller.
			* @param result The result that will be returned to the caller.
			*/
			virtual void get«attributeName.toFirstUpper»(
					joynr::RequestStatus& joynrInternalStatus,
					«returnType»& result
			)«IF pure»=0«ENDIF»;
			
		«ENDFOR»
	'''		

	def produceAsyncGetters(FInterface serviceInterface, boolean pure)
	'''
		«FOR attribute: getAttributes(serviceInterface)»
			«val returnType = getMappedDatatypeOrList(attribute)»
			«val attributeName = attribute.joynrName»
			 
			/**
			* @brief Asynchronous getter for the «attributeName» attribute.
			*
			«printCallbackParamDefinition»
			*/
			virtual void get«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ICallback<«returnType»> > callback
			)«IF pure»=0«ENDIF»;
			 
			/**
			* @brief Asynchronous getter for the location attribute.
			*
			* @param future With this object, you will be able to query the joynrInternalStatus and result of
			* the request.
			«printCallbackParamDefinition»
			«printFutureParamDefinition»
			*/
			virtual void get«attributeName.toFirstUpper»(
					QSharedPointer<joynr::Future<«returnType»> > future,
					QSharedPointer<joynr::ICallback<«returnType»> > callback
			)«IF pure»=0«ENDIF»;
			 
			/**
			* @brief Asynchronous getter for the location attribute.
			*
			«printFutureParamDefinition»
			*/
			virtual void get«attributeName.toFirstUpper»(
					QSharedPointer<joynr::Future<«returnType»> > future
			)«IF pure»=0«ENDIF»;
		«ENDFOR»
	'''		

	def produceSyncSetters(FInterface serviceInterface, boolean pure)
	'''
		«FOR attribute: getAttributes(serviceInterface)»
			«val returnType = getMappedDatatypeOrList(attribute)»
			«val attributeName = attribute.joynrName»
			 
			/**
			* @brief Synchronous setter for the «attributeName» attribute.
			*
			* @param joynrInternalStatus The joynrInternalStatus of the request which will be returned to the caller.
			* @param «returnType.toFirstLower» The value to set.
			*/
			virtual void set«attributeName.toFirstUpper»(
					joynr::RequestStatus& joynrInternalStatus,
					const «returnType»& «attributeName.toFirstLower»
			)«IF pure»=0«ENDIF»;
		«ENDFOR»
	'''		

	def produceAsyncSetters(FInterface serviceInterface, boolean pure)
	'''
		«FOR attribute: getAttributes(serviceInterface)»
			«val returnType = getMappedDatatypeOrList(attribute)»
			«val attributeName = attribute.joynrName»
			 
			/**
			* @brief Asynchronous setter for the «attributeName» attribute.
			*
			«printCallbackParamDefinition»
			* @param «returnType.toFirstLower» The value to set.
			*/
			virtual void set«attributeName.toFirstUpper»(
					QSharedPointer<joynr::ICallback<void> > callback,
					«returnType» «attributeName.toFirstLower»
			)«IF pure»=0«ENDIF»;
			 
			/**
			* @brief Asynchronous setter for the «attributeName» attribute.
			*
			«printFutureParamDefinition»
			«printCallbackParamDefinition»
			* @param «returnType.toFirstLower» The value to set.
			*/
			virtual void set«attributeName.toFirstUpper»(
					QSharedPointer<joynr::Future<void> > future,
					QSharedPointer<joynr::ICallback<void> > callback,
					«returnType» «attributeName.toFirstLower»
			)«IF pure»=0«ENDIF»;
			 
			/**
			* @brief Asynchronous setter for the «attributeName» attribute.
			*
			«printFutureParamDefinition»
			* @param «returnType.toFirstLower» The value to set.
			*/
			virtual void set«attributeName.toFirstUpper»(
					QSharedPointer<joynr::Future<void> > future,
					«returnType» «attributeName.toFirstLower»
			)«IF pure»=0«ENDIF»;
		«ENDFOR»
	'''		
		
	def produceSyncMethods(FInterface serviceInterface, boolean pure)
	'''
		«FOR method: getMethods(serviceInterface)»
			«IF getMappedOutputParameter(method).head=="void"»
				 
				/**
				* @brief Synchronous operation «method.joynrName».
				*
				* @param joynrInternalStatus The joynrInternalStatus of the request which will be returned to the caller.
				*/
				virtual void «method.joynrName»(
						joynr::RequestStatus& joynrInternalStatus «prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
				)«IF pure»=0«ENDIF»;
			«ELSE»
				 
				/**
				* @brief Synchronous operation «method.joynrName».
				*
				* @param joynrInternalStatus The joynrInternalStatus of the request which will be returned to the caller.
				* @param result The result that will be returned to the caller.
				*/
				virtual void «method.joynrName»(
						joynr::RequestStatus& joynrInternalStatus «prependCommaIfNotEmpty(getCommaSeperatedTypedOutputParameterList(method))»«prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
				)«IF pure»=0«ENDIF»;
			«ENDIF»
		«ENDFOR»	
	'''

	def produceAsyncMethods(FInterface serviceInterface, boolean pure)
	'''
		«FOR method: getMethods(serviceInterface)»
			«var returnType = getMappedOutputParameter(method).head» 
			 
			/**
			* @brief Asynchronous operation «method.joynrName».
			*
			«printCallbackParamDefinition»
			*/
			virtual void «method.joynrName»(
					QSharedPointer<joynr::ICallback<«returnType»> > callback «prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			)«IF pure»=0«ENDIF»;
			 
			/**
			* @brief Asynchronous operation «method.joynrName».
			*
			«printFutureParamDefinition»
			«printCallbackParamDefinition»
			*/
			virtual void «method.joynrName»(
					QSharedPointer<joynr::Future<«returnType»> > future,
					QSharedPointer<joynr::ICallback<«returnType»> > callback «prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			)«IF pure»=0«ENDIF»;
			 
			/** 
			* @brief Asynchronous operation «method.joynrName».
			«printFutureParamDefinition»
			«printCallbackParamDefinition»
			*/
			
			virtual void «method.joynrName»(
					QSharedPointer<joynr::Future<«returnType»> > future «prependCommaIfNotEmpty(getCommaSeperatedTypedParameterList(method))»
			)«IF pure»=0«ENDIF»;
		«ENDFOR»	
	'''	
}