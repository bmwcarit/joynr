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
import io.joynr.generator.js.util.GeneratorParameter
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
	@Inject extension GeneratorParameter
	@Inject private extension NamingUtil
	@Inject private extension MethodUtil
	@Inject private extension InterfaceUtil

	def relativePathToBase() {
		var relativePath = ""
		for (var i=0; i<packagePathDepth; i++) {
			relativePath += ".." + File::separator
		}
		return relativePath
	}

	def generateProvider(IFileSystemAccess fsa){
		var fileName = path + "" + providerName + ".js"
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
	/*
	 * PLEASE NOTE. THIS IS A GENERATED FILE!
	 * Generation date: «generationDate»
	 */
	(function(undefined){

		var checkImpl = function() {
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
		};

		/**
		 * @name «providerName»
		 * @constructor
		 *
		 * @classdesc
		 * PLEASE NOTE. THIS IS A GENERATED FILE!
		 * <br/>Generation date: «generationDate»
		 «appendJSDocSummaryAndWriteSeeAndDescription(francaIntf, "* ")»
		 *
		 * @summary Constructor of «providerName» object
		 *
		 * @param {Object} [implementation] the implementation of the provider
		 * @param {Object} [implementation.ATTRIBUTENAME] the definition of attribute implementation
		 * @param {Function} [implementation.ATTRIBUTENAME.set] the getter function with the
		 *     signature "function(value){}" that stores the given attribute value
		 * @param {Function} [implementation.ATTRIBUTENAME.get] the getter function with the
		 *     signature "function(){}" that returns the current attribute value
		 * @param {Function} [implementation.OPERATIONNAME] the operation function
		 * @param {Object} [implementation.EVENTNAME] the definition of the event implementation
		 *
		 * @returns {«providerName»} a «providerName» object to communicate with the joynr infrastructure
		 */
		var «providerName» = null;
		«providerName» = function «providerName»(
			passedImplementation,
			dependencies
		) {
			if (!(this instanceof «providerName»)) {
				// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
				return new «providerName»(
					passedImplementation,
					dependencies);
			}

			var implementation = passedImplementation || {};

			// defining provider members
			«FOR attribute: getAttributes(francaIntf)»
				«val attributeName = attribute.joynrName»
				/**
				 * @name «providerName»#«attributeName»
				 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
				 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
				 */
				this.«attributeName» = new dependencies.ProviderAttribute«getAttributeCaps(attribute)»
					(this, implementation.«attributeName», "«attributeName»", "«attribute.joynrTypeName»");
				if (implementation.«attributeName») {
					implementation.«attributeName».valueChanged = this.«attributeName».valueChanged;
				}
			«ENDFOR»
			«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
			«FOR methodName : getMethodNames(francaIntf)»
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
				this.«methodName» = new dependencies.ProviderOperation(this, implementation.«methodName», "«methodName»", [
					«FOR operation: getMethods(francaIntf, methodName) SEPARATOR ","»
					{
						inputParameter: [
							«FOR param: getInputParameters(operation) SEPARATOR ","»
							{
								name : "«param.joynrName»",
								type : "«param.joynrTypeName»",
								javascriptType : "«param.javascriptTypeName»"
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
								type : "«param.joynrTypeName»",
								javascriptType : "«param.javascriptTypeName»"
							}
							«ENDFOR»
						]
					}
					«ENDFOR»
				]);
			«ENDFOR»
			«FOR event: getEvents(francaIntf)»
				«val filterParameters = getFilterParameters(event)»
				«val eventName = event.joynrName»
				/**
				 * @name «providerName»#«eventName»
				 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
				 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
				 */
				this.«eventName» = new dependencies.ProviderEvent({
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
				if (implementation.«eventName») {
					implementation.«eventName».createBroadcastOutputParameters = this.«eventName».createBroadcastOutputParameters;
					implementation.«eventName».fire = this.«eventName».fire;
					implementation.«eventName».addBroadcastFilter = this.«eventName».addBroadcastFilter;
				}
			«ENDFOR»

			Object.defineProperty(this, 'checkImplementation', {
				enumerable: false,
				configurable: false,
				writable: false,
				value: checkImpl
			});

			this.interfaceName = "«getFQN(francaIntf)»";

			return Object.freeze(this);
		};

		/**
		 * @name «providerName»#MAJOR_VERSION
		 * @constant {Number}
		 * @default «majorVersion»
		 * @summary The MAJOR_VERSION of the provider is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		Object.defineProperty(«providerName», 'MAJOR_VERSION', {
			enumerable: false,
			configurable: false,
			writable: false,
			readable: true,
			value: «majorVersion»
		});
		/**
		 * @name «providerName»#MINOR_VERSION
		 * @constant {Number}
		 * @default «minorVersion»
		 * @summary The MINOR_VERSION of the provider is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		Object.defineProperty(«providerName», 'MINOR_VERSION', {
			enumerable: false,
			configurable: false,
			writable: false,
			readable: true,
			value: «minorVersion»
		});

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«francaIntf.defineName(providerName)»[
				«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes) SEPARATOR ','»
						"«datatype.getDependencyPath»"
				«ENDFOR»
				], function () {
					return «providerName»;
				}
			);
		} else if (typeof exports !== 'undefined' ) {
			if ((module !== undefined) && module.exports) {
				«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes)»
					require("«relativePathToBase() + datatype.getDependencyPath()»");
				«ENDFOR»
				exports = module.exports = «providerName»;
			}
			else {
				// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
				exports.«providerName» = «providerName»;
			}
		} else {
			window.«providerName» = «providerName»;
		}
		«ELSE»
		window.«providerName» = «providerName»;
		«ENDIF»
	})();
	'''

	def determErrorTypeName(FMethod method, String errorEnumName) {
		var enumType = method.errors;
		if (enumType != null) {
			enumType.name = errorEnumName;
			return getTypeNameForErrorEnumType(method, enumType);
		}
		else if (method.errorEnum != null){
			return method.errorEnum.joynrTypeName;
		}
		else {
			return "no error enumeration given"
		}
	}

}
