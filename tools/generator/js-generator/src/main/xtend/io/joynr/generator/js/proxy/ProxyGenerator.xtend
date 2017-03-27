package io.joynr.generator.js.proxy

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
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import java.util.Date
import org.eclipse.xtext.generator.IFileSystemAccess

class ProxyGenerator extends InterfaceJsTemplate {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension JSTypeUtil
	@Inject extension GeneratorParameter
	@Inject private extension NamingUtil
	@Inject private extension MethodUtil
	@Inject private extension BroadcastUtil
	@Inject private extension InterfaceUtil

	def relativePathToBase() {
		var relativePath = ""
		for (var i=0; i<packagePathDepth; i++) {
			relativePath += ".." + File::separator
		}
		return relativePath
	}

	def generateProxy(IFileSystemAccess fsa){
		var fileName = path + "" + proxyName + ".js"
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

	def proxyName(){
		francaIntf.joynrName + "Proxy"
	}

	override generate()'''
	«val generationDate = (new Date()).toString»
	/**
	 * PLEASE NOTE: THIS IS A GENERATED FILE!
	 * Generation date: «generationDate»
	 *
	 * «proxyName», generated from the corresponding interface description.
	 */
	(function (undefined){
		/**
		 * @name «proxyName»
		 * @constructor
		 *
		 * @classdesc
		 * <br/>Generation date: «generationDate»
		 * <br/><br/>
		 * «proxyName», generated from the corresponding interface description.
		 «appendJSDocSummaryAndWriteSeeAndDescription(francaIntf, "* ")»
		 *
		 * @param {object} settings the settings object for this function call
		 * @param {String} settings.domain the domain name //TODO: check do we need this?
		 * @param {String} settings.joynrName the interface name //TODO: check do we need this?
		 *
		 * @param {Object} settings.discoveryQos the Quality of Service parameters for arbitration
		 * @param {Number} settings.discoveryQos.discoveryTimeoutMs for rpc calls to wait for arbitration to finish.
		 * @param {String} settings.discoveryQos.arbitrationStrategy Strategy for choosing the appropriate provider from the list returned by the capabilities directory
		 * @param {Number} settings.discoveryQos.cacheMaxAgeMs Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
		 * @param {Boolean} settings.discoveryQos.discoveryScope If localOnly is set to true, only local providers will be considered.
		 * @param {Object} settings.discoveryQos.additionalParameters a map holding additional parameters in the form of key value pairs in the javascript object, e.g.: {"myKey": "myValue", "myKey2": 5}
		 *
		 * @param {object} settings.messagingQos the Quality of Service parameters for messaging
		 * @param {Number} settings.messagingQos.ttl Roundtrip timeout for rpc requests.

		 * @param {Number} settings.dependencies instances of the internal objects needed by the proxy to interface with joynr
		 * @param {Number} settings.proxyElementTypes constructors for attribute, method and broadcasts, used to create the proxy's elements
		 *
		 * @returns {«proxyName»} a «proxyName» object to access other providers
		 */
		var «proxyName» = function «proxyName»(
			settings) {
			if (!(this instanceof «proxyName»)) {
				// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
				return new «proxyName»(
					settings);
			}

			// generated package name
			this.settings = settings || {};

	«FOR attribute : getAttributes(francaIntf)»
	«val attributeName = attribute.joynrName»
			/**
			 * @name «proxyName»#«attributeName»
			 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
			*/
			this.«attributeName» = new settings.proxyElementTypes.ProxyAttribute«getAttributeCaps(attribute)»(this, settings, "«attributeName»", "«attribute.joynrTypeName»");
	«ENDFOR»

	«FOR operationName : getMethodNames(francaIntf)»
	«val operations = getMethods(francaIntf, operationName)»
			«FOR operation : operations»
				/**
				 * @function «proxyName»#«operationName»
				 * @summary The «operationName» operation is GENERATED FROM THE INTERFACE DESCRIPTION
				 «IF operations.size > 1»
				 * <br/>method overloading: different call semantics possible
				 «ENDIF»
				 «appendJSDocSummaryAndWriteSeeAndDescription(operation, "* ")»
				 *
				 «writeJSDocForSignature(proxyName, operation, "* ")»
				 */
				«IF operation.outputParameters.size>0»
					/**
					 «writeJSDocTypedefForSignature(proxyName, operation, operationName, "* ")»
					 */
				«ENDIF»
			«ENDFOR»
			this.«operationName» = new settings.proxyElementTypes.ProxyOperation(this, settings, "«operationName»", [
				«FOR operation: getMethods(francaIntf, operationName) SEPARATOR ","»
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
					outputParameter: [
						«FOR param: getOutputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : "«param.joynrTypeName»",
							javascriptType : "«param.javascriptTypeName»"
						}
						«ENDFOR»
					],
					fireAndForget: «IF operation.fireAndForget»true«ELSE»false«ENDIF»
				}«ENDFOR»
			]).buildFunction();
	«ENDFOR»

	«FOR event: getEvents(francaIntf)»
	«val eventName = event.joynrName»
	«val filterParameters = getFilterParameters(event)»
			/**
			 * @name «proxyName»#«eventName»
			 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
			 */
			this.«eventName» = new settings.proxyElementTypes.ProxyEvent(this, {
					broadcastName : "«eventName»",
					broadcastParameter : [
						«FOR param: event.outputParameters SEPARATOR ", "»
						{
							name : "«param.joynrName»",
							type : "«param.joynrTypeName»"
						}
						«ENDFOR»
					],
					messagingQos : settings.messagingQos,
					discoveryQos : settings.discoveryQos,
					«IF event.selective»
					dependencies: {
							subscriptionManager: settings.dependencies.subscriptionManager
					},
					selective : «event.selective»,
					«IF filterParameters.size > 0»
					filterParameters: {
						«FOR filterParameter : filterParameters SEPARATOR ","»
							"«filterParameter»": "reservedForTypeInfo"
						«ENDFOR»
					}
					«ENDIF»
					«ELSE»
					dependencies: {
							subscriptionManager: settings.dependencies.subscriptionManager
					}
					«ENDIF»
				});
	«ENDFOR»

			Object.defineProperty(this, "interfaceName", {
				writable: false,
				readable: true,
				value: "«getFQN(francaIntf)»"
			});
		};

		/**
		 * @name «proxyName»#MAJOR_VERSION
		 * @constant {Number}
		 * @default «majorVersion»
		 * @summary The MAJOR_VERSION of the proxy is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		Object.defineProperty(«proxyName», 'MAJOR_VERSION', {
			enumerable: false,
			configurable: false,
			writable: false,
			readable: true,
			value: «majorVersion»
		});
		/**
		 * @name «proxyName»#MINOR_VERSION
		 * @constant {Number}
		 * @default «minorVersion»
		 * @summary The MINOR_VERSION of the proxy is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		Object.defineProperty(«proxyName», 'MINOR_VERSION', {
			enumerable: false,
			configurable: false,
			writable: false,
			readable: true,
			value: «minorVersion»
			});

		«proxyName».getUsedDatatypes = function getUsedDatatypes(){
			return [
						«FOR datatype : francaIntf.getAllComplexTypes SEPARATOR ','»
						"«datatype.joynrTypeName»"
						«ENDFOR»
					];
		};

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«francaIntf.defineName(proxyName)»[
				«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes) SEPARATOR ','»
						"«datatype.getDependencyPath»"
				«ENDFOR»
				], function () {
					return «proxyName»;
				});
		} else if (typeof exports !== 'undefined' ) {
			if ((module !== undefined) && module.exports) {
				«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes)»
					require("«relativePathToBase() + datatype.getDependencyPath()»");
				«ENDFOR»
				exports = module.exports = «proxyName»;
			}
			else {
				// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
				exports.«proxyName» = «proxyName»;
			}
		} else {
			window.«proxyName» = «proxyName»;
		}
		«ELSE»
		window.«proxyName» = «proxyName»;
		«ENDIF»
	})();
	'''

}
