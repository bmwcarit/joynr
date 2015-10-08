package io.joynr.generator.js.provider

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
import io.joynr.generator.js.util.GeneratorParameter
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import java.io.File
import java.util.Date
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType
import io.joynr.generator.js.util.JSTypeUtil

class ProviderGenerator {

	@Inject
	extension JoynrJSGeneratorExtensions

	@Inject
	extension JSTypeUtil

	@Inject
	extension GeneratorParameter

	def generateProvider(FInterface fInterface, Iterable<FType> types, IFileSystemAccess fsa){
		var containerpath = File::separator

		val path = containerpath +
			getPackagePathWithJoynrPrefix(fInterface, File::separator) +
			File::separator

		val fileName = path + "" + fInterface.providerName + ".js"
		if (clean) {
			fsa.deleteFile(fileName)
		}
		if (generate) {
			fsa.generateFile(
				fileName,
				generate(fInterface, types).toString
			)
		}
	}

	def getProviderName(FInterface fInterface){
		fInterface.joynrName + "Provider"
	}

	def generate(FInterface fInterface, Iterable<FType> types)'''
	«val generationDate = (new Date()).toString»
	/*
	 * PLEASE NOTE. THIS IS A GENERATED FILE!
	 * Generation date: «generationDate»
	 */
	(function(undefined){

		var checkImpl = function() {
			var missingInImplementation = [];
			«FOR attribute: getAttributes(fInterface)»
			«val attributeName = attribute.joynrName»
			if (! this.«attributeName».check()) missingInImplementation.push("Attribute:«attributeName»");
			«ENDFOR»
			«FOR methodName : getMethodNames(fInterface)»
			if (! this.«methodName».checkOperation())  missingInImplementation.push("Operation:«methodName»");
			«ENDFOR»
			«FOR event: getEvents(fInterface)»
			// check if implementation was provided for «event.joynrName»
			«ENDFOR»

			return missingInImplementation;
		};

		/**
		 * @name «fInterface.providerName»
		 * @constructor
		 *
		 * @classdesc
		 * PLEASE NOTE. THIS IS A GENERATED FILE!
		 * <br/>Generation date: «generationDate»
		 «appendJSDocSummaryAndWriteSeeAndDescription(fInterface, "* ")»
		 *
		 * @summary Constructor of «fInterface.providerName» object
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
		 * @returns {«fInterface.providerName»} a «fInterface.providerName» object to communicate with the joynr infrastructure
		 */
		var «fInterface.providerName» = null;
		«fInterface.providerName» = function «fInterface.providerName»(
			passedImplementation,
			dependencies
		) {
			if (!(this instanceof «fInterface.providerName»)) {
				// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
				return new «fInterface.providerName»(
					passedImplementation,
					dependencies);
			}

			var implementation = passedImplementation || {};
			var TypesEnum = dependencies.TypesEnum;

			// defining provider members
			«FOR attribute: getAttributes(fInterface)»
				«val attributeName = attribute.joynrName»
				/**
				 * @name «fInterface.providerName»#«attributeName»
				 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
				 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
				 */
				this.«attributeName» = new dependencies.ProviderAttribute«getAttributeCaps(attribute)»
					(this, implementation.«attributeName», "«attributeName»", «attribute.getTypeNameForParameter(true)»);
				if (implementation.«attributeName») {
					implementation.«attributeName».valueChanged = this.«attributeName».valueChanged;
				}
			«ENDFOR»
			«FOR methodName : getMethodNames(fInterface)»
				«val operations = getMethods(fInterface, methodName)»
				«FOR operation : operations»
					/**
					 * @function «fInterface.providerName»#«methodName»
					 * @summary The «methodName» operation is GENERATED FROM THE INTERFACE DESCRIPTION
					 «IF operations.size > 1»
					 * <br/>method overloading: different call semantics possible
					 «ENDIF»
					 «appendJSDocSummaryAndWriteSeeAndDescription(operation, "* ")»
					 *
					 «writeJSDocForSignature(operation, "* ")»
					 */
				«ENDFOR»
				this.«methodName» = new dependencies.ProviderOperation(this, implementation.«methodName», "«methodName»", [
					«FOR operation: getMethods(fInterface, methodName) SEPARATOR ","»
					{
						inputParameter: [
							«FOR param: getInputParameters(operation) SEPARATOR ","»
							{
								name : "«param.joynrName»",
								type : «param.getTypeNameForParameter(true)»
							}
							«ENDFOR»
						],
						outputParameter: [
							«FOR param: getOutputParameters(operation) SEPARATOR ","»
							{
								name : "«param.joynrName»",
								type : «param.getTypeNameForParameter(true)»
							}
							«ENDFOR»
						]
					}
					«ENDFOR»
				]);
			«ENDFOR»
			«FOR event: getEvents(fInterface)»
				«val filterParameters = getFilterParameters(event)»
				«val eventName = event.joynrName»
				/**
				 * @name «fInterface.providerName»#«eventName»
				 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
				 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
				 */
				this.«eventName» = new dependencies.ProviderEvent(
					this,
					implementation.«eventName»,
					"«eventName»",
					[
						«FOR param : getOutputParameters(event) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : «param.typeNameForParameter»
						}
						«ENDFOR»
					],
					«IF isSelective(event)»
					{
						«FOR filterParameter : filterParameters SEPARATOR ","»
							"«filterParameter»": "reservedForTypeInfo"
						«ENDFOR»
					}
					«ELSE»
					{}
					«ENDIF»
				);
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

			this.interfaceName = "«getFQN(fInterface)»";

			this.id = dependencies.uuid();

			return Object.freeze(this);
		};

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«fInterface.defineName(fInterface.providerName)»[], function () {
					return «fInterface.providerName»;
				}
			);
		} else if (typeof exports !== 'undefined' ) {
			if ((module !== undefined) && module.exports) {
				exports = module.exports = «fInterface.providerName»;
			}
			else {
				// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
				exports.«fInterface.providerName» = «fInterface.providerName»;
			}
		} else {
			window.«fInterface.providerName» = «fInterface.providerName»;
		}
		«ELSE»
		window.«fInterface.providerName» = «fInterface.providerName»;
		«ENDIF»
	})();
	'''
}
