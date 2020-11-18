package io.joynr.generator.js.provider

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
import io.joynr.generator.js.templates.InterfaceJsTemplate
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import java.util.Date
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FMethod

class ProviderGenerator extends InterfaceJsTemplate {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension JSTypeUtil
	@Inject extension NamingUtil
	@Inject extension MethodUtil
	@Inject extension InterfaceUtil

	def relativePathToBase() {
		var relativePath = ""
		for (var i=0; i<packagePathDepth; i++) {
			relativePath += ".." + File::separator
		}
		return relativePath
	}

	def generateProvider(IFileSystemAccess fsa, boolean generateVersion){
		super.generate(generateVersion)
		var fileName = path + "" + providerName + ".ts"
		if (clean) {
			fsa.deleteFile(fileName)
		}
		if (generate) {
			fsa.generateFile(
				fileName,
				generate(generateVersion).toString
			)
		}
	}

	def getProviderName(){
		francaIntf.joynrName + "Provider"
	}

	override generate(boolean generateVersion)'''
	«val generationDate = (new Date()).toString»
	«val attributes = getAttributes(francaIntf)»
	«val methodNames = getMethodNames(francaIntf)»
	«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
	«val events = getEvents(francaIntf)»
	«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes)»
	import «datatype.joynrName» = require("«relativePathToBase() + datatype.getDependencyPath(generateVersion)»");
	«ENDFOR»
	«IF attributes.length > 0»
	import {«FOR attributeType: attributes.providerAttributeNames SEPARATOR ','»«attributeType» «ENDFOR»} from "joynr/joynr/provider/ProviderAttribute";
	«ENDIF»
	«IF methodNames.length > 0»
	import ProviderOperation = require("joynr/joynr/provider/ProviderOperation");
	«ENDIF»
	«IF events.length > 0»
	import ProviderEvent = require("joynr/joynr/provider/ProviderEvent");
	«ENDIF»
	import {«FOR attributeType: attributes.joynrProviderImports SEPARATOR ','»«attributeType» «ENDFOR»} from "joynr/joynr/types/JoynrProvider";

	namespace «providerName» {
		export interface «providerName»Implementation {
			«FOR attribute: attributes»
				«val attributeName = attribute.joynrName»
				«attributeName»: «attribute.providerAttributeImplName»<«attribute.tsTypeName»>;
			«ENDFOR»
			«FOR methodName : methodNames»
				«methodName»: «methodName.toFirstUpper»Signature;
			«ENDFOR»
			«FOR event: events»
				/** Broadcast.fire methods will be added to implementation during provider registration **/
				«event.joynrName»?: ProviderEvent
			«ENDFOR»
		}

		«FOR operationName : methodNames»
			«val operations = getMethods(francaIntf, operationName)»
			«FOR i : 1..operations.size»
				«val operation = operations.get(i - 1)»
				«IF getInputParameters(operation).length == 0»
					export type «operationName.toFirstUpper»Args«i» = void;
				«ELSE»
					export interface «operationName.toFirstUpper»Args«i» {«
						FOR param: getInputParameters(operation)»«
							param.joynrName»: «param.tsTypeName»;«ENDFOR» }
				«ENDIF»
				«IF getOutputParameters(operation).length == 0»
					export type «operationName.toFirstUpper»Returns«i» = void;
				«ELSE»
					export interface «operationName.toFirstUpper»Returns«i» {«
						FOR param: getOutputParameters(operation)»«
							param.joynrName»: «param.tsTypeName»; «ENDFOR»}
				«ENDIF»
			«ENDFOR»
			export type «operationName.toFirstUpper»Signature = «FOR i : 1..operations.size BEFORE "(" SEPARATOR ")&(" AFTER ")"
			»(settings: «operationName.toFirstUpper»Args«i») => «operationName.toFirstUpper»Returns«i» | Promise<«operationName.toFirstUpper»Returns«i»>«
			ENDFOR»
		«ENDFOR»
	}

	/**
	 * PLEASE NOTE. THIS IS A GENERATED FILE!
	 * <br/>Generation date: «generationDate»
	 «appendJSDocSummaryAndWriteSeeAndDescription(francaIntf, "* ")»
	 */
	class «providerName» extends JoynrProvider {

		public interfaceName = "«francaIntf.fullyQualifiedName»";
		«FOR attribute: attributes»
			«val attributeName = attribute.joynrName»
			/**
			 * @name «providerName»#«attributeName»
			 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
			 */
			public «attributeName»: «attribute.providerAttributeName»<«attribute.tsTypeName»>;
		«ENDFOR»
	
		«FOR methodName : methodNames»
			«val operations = getMethods(francaIntf, methodName)»
			«FOR operation : operations»
			/**
			 * @function «providerName»#«methodName»
			 * @summary The «methodName» operation is GENERATED FROM THE INTERFACE DESCRIPTION
			 «IF operations.size > 1»
			 * <br/>method overloading: different call semantics possible
			 «ENDIF»
			 «appendJSDocSummaryAndWriteSeeAndDescription(operation, "* ")»
			 *
			 «writeJSDocForSignature(providerName, operation, "* ")»
			 */
			«IF operation.outputParameters.size>0»
			/**
			 «writeJSDocTypedefForSignature(providerName, operation, methodName, "* ")»
			 */
			«ENDIF»
			«ENDFOR»
			public «methodName»: ProviderOperation;
		«ENDFOR»

		«FOR event: events»
			«val eventName = event.joynrName»
			/**
			 * @name «providerName»#«eventName»
			 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
			 */
			public «eventName»: ProviderEvent;
		«ENDFOR»

		/**
		 * @param [implementation] the implementation of the provider
		 * @param [implementation.ATTRIBUTENAME] the definition of attribute implementation
		 * @param [implementation.ATTRIBUTENAME.set] the getter function that stores the given attribute value
		 * @param [implementation.ATTRIBUTENAME.get] the getter function that returns the current attribute value
		 * @param [implementation.OPERATIONNAME] the operation function
		 * @param [implementation.EVENTNAME] the definition of the event implementation
		 */
		public constructor(implementation: «providerName».«providerName»Implementation) {
			super();
			// defining provider members
			«FOR attribute: attributes»
				«val attributeName = attribute.joynrName»
				this.«attributeName» = new «attribute.providerAttributeName»<«attribute.tsTypeName»>(this, implementation.«attributeName», "«attributeName»", "«attribute.getJoynrTypeName(generateVersion)»");
			«ENDFOR»
	
			«FOR methodName : methodNames»
				this.«methodName» = new ProviderOperation(implementation.«methodName», "«methodName»", [
					«FOR operation: getMethods(francaIntf, methodName) SEPARATOR ","»
					{
						inputParameter: [
							«FOR param: getInputParameters(operation) SEPARATOR ","»
								{
									name : "«param.joynrName»",
									type : "«param.getJoynrTypeName(generateVersion)»"
								}
							«ENDFOR»
						],
						error: {
							type: "«determErrorTypeName(operation, methodToErrorEnumName.get(operation), generateVersion)»"
						},
						outputParameter: [
							«FOR param: getOutputParameters(operation) SEPARATOR ","»
								{
									name : "«param.joynrName»",
									type : "«param.getJoynrTypeName(generateVersion)»"
								}
							«ENDFOR»
						]
					}
					«ENDFOR»
				]);
			«ENDFOR»
	
			«FOR event: events»
				«val filterParameters = getFilterParameters(event)»
				«val eventName = event.joynrName»
				this.«eventName» = new ProviderEvent({
					eventName : "«eventName»",
					outputParameterProperties : [
						«FOR param : getOutputParameters(event) SEPARATOR ","»
							{
								name : "«param.joynrName»",
								type : "«param.getJoynrTypeName(generateVersion)»"
							}
						«ENDFOR»
					],
					selective : «event.selective»,
					filterSettings : {
					«IF event.selective»
						«FOR filterParameter : filterParameters SEPARATOR ","»
							"«filterParameter»": "reservedForTypeInfo"
						«ENDFOR»
					«ENDIF»
					}
				});
				implementation.«eventName» = this.«eventName»;
			«ENDFOR»
		}

		/**
		 * The MAJOR_VERSION of the provider is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MAJOR_VERSION = «majorVersion»;

		/**
		 * The MINOR_VERSION of the provider is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MINOR_VERSION = «minorVersion»;

		public static getUsedJoynrtypes(): any[] {
			return [
				«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes) SEPARATOR ','»
					«datatype.joynrName»
				«ENDFOR»
			];
		}
	}
	export = «providerName»;

	'''

	def determErrorTypeName(FMethod method, String errorEnumName, boolean generateVersion) {
		var enumType = method.errors;
		if (enumType !== null) {
			enumType.name = errorEnumName;
			return getTypeNameForErrorEnumType(method, enumType, generateVersion);
		}
		else if (method.errorEnum !== null){
			return method.errorEnum.getJoynrTypeName(generateVersion);
		}
		else {
			return "no error enumeration given"
		}
	}

}
