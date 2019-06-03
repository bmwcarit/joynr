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

	def generateProvider(IFileSystemAccess fsa){
		var fileName = path + "" + providerName + ".ts"
		if (clean) {
			fsa.deleteFile(fileName)
		}
		if (generate) {
			fsa.generateFile(
				fileName,
				generate().toString
			)
		}
	}

	def getProviderName(){
		francaIntf.joynrName + "Provider"
	}

	override generate()'''
	«val generationDate = (new Date()).toString»
	«val attributes = getAttributes(francaIntf)»
	«val methodNames = getMethodNames(francaIntf)»
	«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
	«val events = getEvents(francaIntf)»

	«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes)»
	import «datatype.name» from "«relativePathToBase() + datatype.getDependencyPath()»";
	«ENDFOR»

	«IF attributes.length > 0»
	import {«FOR attributeType: attributes.providerAttributeNames SEPARATOR ','»«attributeType» «ENDFOR»} from "joynr/joynr/provider/ProviderAttribute";
	«ENDIF»
	«IF methodNames.length > 0»
	import ProviderOperation from "joynr/joynr/provider/ProviderOperation"
	«ENDIF»
	«IF events.length > 0»
	import ProviderEvent from "joynr/joynr/provider/ProviderEvent"
	«ENDIF»

	/**
	 * PLEASE NOTE. THIS IS A GENERATED FILE!
	 * <br/>Generation date: «generationDate»
	 «appendJSDocSummaryAndWriteSeeAndDescription(francaIntf, "* ")»
	 */
	class «providerName» {

		public interfaceName = "«francaIntf.fullyQualifiedName»";
	«FOR attribute: attributes»
		«val attributeName = attribute.joynrName»
		/**
		 * @name «providerName»#«attributeName»
		 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
		 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
		 */
		public «attributeName»: «attribute.providerAttributeName»;
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
		 * @param {Object} [implementation] the implementation of the provider
		 * @param {Object} [implementation.ATTRIBUTENAME] the definition of attribute implementation
		 * @param {Function} [implementation.ATTRIBUTENAME.set] the getter function with the
		 *     signature "function(value){}" that stores the given attribute value
		 * @param {Function} [implementation.ATTRIBUTENAME.get] the getter function with the
		 *     signature "function(){}" that returns the current attribute value
		 * @param {Function} [implementation.OPERATIONNAME] the operation function
		 * @param {Object} [implementation.EVENTNAME] the definition of the event implementation
		 */
		public constructor(implementation: any = {}) {
			// defining provider members
		«FOR attribute: attributes»
			«val attributeName = attribute.joynrName»
			this.«attributeName» = new «attribute.providerAttributeName»(this, implementation.«attributeName», "«attributeName»", "«attribute.joynrTypeName»");
			if (implementation.«attributeName») {
				implementation.«attributeName».valueChanged = this.«attributeName».valueChanged;
			}
		«ENDFOR»

		«FOR methodName : methodNames»
			this.«methodName» = new ProviderOperation(this, implementation.«methodName», "«methodName»", [
				«FOR operation: getMethods(francaIntf, methodName) SEPARATOR ","»
				{
					inputParameter: [
						«FOR param: getInputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : "«param.joynrTypeName»"
						}
						«ENDFOR»
					],
					error: {
						type: "«determErrorTypeName(operation, methodToErrorEnumName.get(operation))»"
					},
					outputParameter: [
						«FOR param: getOutputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : "«param.joynrTypeName»"
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
						type : "«param.joynrTypeName»"
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

		public checkImplementation(): string[] {
			var missingInImplementation = [];
			«FOR attribute: getAttributes(francaIntf)»
			«val attributeName = attribute.joynrName»
			if (! this.«attributeName».check()) missingInImplementation.push("Attribute:«attributeName»");
			«ENDFOR»
			«FOR methodName : getMethodNames(francaIntf)»
			if (! this.«methodName».checkOperation())  missingInImplementation.push("Operation:«methodName»");
			«ENDFOR»
			«FOR event: getEvents(francaIntf)»
			// check if implementation was provided for «event.joynrName»
			«ENDFOR»

			return missingInImplementation;
		}

		/**
		 * The MAJOR_VERSION of the provider is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MAJOR_VERSION = { value: «majorVersion» };

		/**
		 * The MINOR_VERSION of the provider is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MINOR_VERSION = { value: «minorVersion» };

		public static getUsedJoynrtypes(): any[] {
			return [
				«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes) SEPARATOR ','»
				«datatype.name»
				«ENDFOR»
			];
		}
	}

	export = «providerName»;
	'''

	def determErrorTypeName(FMethod method, String errorEnumName) {
		var enumType = method.errors;
		if (enumType !== null) {
			enumType.name = errorEnumName;
			return getTypeNameForErrorEnumType(method, enumType);
		}
		else if (method.errorEnum !== null){
			return method.errorEnum.joynrTypeName;
		}
		else {
			return "no error enumeration given"
		}
	}

}
