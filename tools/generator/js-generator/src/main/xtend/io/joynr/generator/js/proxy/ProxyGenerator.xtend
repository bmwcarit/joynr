package io.joynr.generator.js.proxy

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
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import java.util.Date
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.franca.core.franca.FType

class ProxyGenerator {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension JSTypeUtil
	@Inject extension GeneratorParameter
	@Inject private extension NamingUtil
	@Inject private extension MethodUtil
	@Inject private extension BroadcastUtil
	@Inject private extension InterfaceUtil

	int packagePathDepth

	def relativePathToBase() {
		var relativePath = ""
		for (var i=0; i<packagePathDepth; i++) {
			relativePath += ".." + File::separator
		}
		return relativePath
	}

	def getDependencyPath(FType datatype) {
		return datatype.buildPackagePath(File.separator, true)
					+ File.separator
					+ datatype.joynrName
	}

	def generateProxy(FInterface fInterface, Iterable<FType> types, IFileSystemAccess fsa){
		var containerpath = File::separator //+ "generated" + File::separator

		val packagePath = getPackagePathWithJoynrPrefix(fInterface, File::separator)
		val path = containerpath + packagePath + File::separator
		packagePathDepth = packagePath.split(File::separator).length

		val fileName = path + "" + fInterface.proxyName + ".js"
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

	def proxyName(FInterface fInterface){
		fInterface.joynrName + "Proxy"
	}

	def generate(FInterface fInterface, Iterable<FType> types)'''
	«val generationDate = (new Date()).toString»
	/**
	 * PLEASE NOTE: THIS IS A GENERATED FILE!
	 * Generation date: «generationDate»
	 *
	 * «fInterface.proxyName», generated from the corresponding interface description.
	 */
	(function (undefined){
		/**
		 * @name «fInterface.proxyName»
		 * @constructor
		 *
		 * @classdesc
		 * <br/>Generation date: «generationDate»
		 * <br/><br/>
		 * «fInterface.proxyName», generated from the corresponding interface description.
		 «appendJSDocSummaryAndWriteSeeAndDescription(fInterface, "* ")»
		 *
		 * @param {object} settings the settings object for this function call
		 * @param {String} settings.domain the domain name //TODO: check do we need this?
		 * @param {String} settings.joynrName the interface name //TODO: check do we need this?
		 *
		 * @param {Object} settings.discoveryQos the Quality of Service parameters for arbitration
		 * @param {Number} settings.discoveryQos.discoveryTimeout for rpc calls to wait for arbitration to finish.
		 * @param {String} settings.discoveryQos.arbitrationStrategy Strategy for choosing the appropriate provider from the list returned by the capabilities directory
		 * @param {Number} settings.discoveryQos.cacheMaxAge Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
		 * @param {Boolean} settings.discoveryQos.discoveryScope If localOnly is set to true, only local providers will be considered.
		 * @param {Object} settings.discoveryQos.additionalParameters a map holding additional parameters in the form of key value pairs in the javascript object, e.g.: {"myKey": "myValue", "myKey2": 5}
		 *
		 * @param {object} settings.messagingQos the Quality of Service parameters for messaging
		 * @param {Number} settings.messagingQos.ttl Roundtrip timeout for rpc requests.

		 * @param {Number} settings.dependencies instances of the internal objects needed by the proxy to interface with joynr
		 * @param {Number} settings.proxyElementTypes constructors for attribute, method and broadcasts, used to create the proxy's elements
		 *
		 * @returns {«fInterface.proxyName»} a «fInterface.proxyName» object to access other providers
		 */
		var «fInterface.proxyName» = function «fInterface.proxyName»(
			settings) {
			if (!(this instanceof «fInterface.proxyName»)) {
				// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
				return new «fInterface.proxyName»(
					settings);
			}

			// generated package name
			this.settings = settings || {};
			var TypesEnum = settings.proxyElementTypes.TypesEnum;

	«FOR attribute : getAttributes(fInterface)»
	«val attributeName = attribute.joynrName»
			/**
			 * @name «fInterface.proxyName»#«attributeName»
			 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
			*/
			//TODO: generate type below (TypesEnum)
			this.«attributeName» = new settings.proxyElementTypes.ProxyAttribute«getAttributeCaps(attribute)»(this, settings, "«attributeName»", «attribute.typeNameForParameter»);
	«ENDFOR»

	«FOR operationName : getMethodNames(fInterface)»
	«val operations = getMethods(fInterface, operationName)»
			«FOR operation : operations»
				/**
				 * @function «fInterface.proxyName»#«operationName»
				 * @summary The «operationName» operation is GENERATED FROM THE INTERFACE DESCRIPTION
				 «IF operations.size > 1»
				 * <br/>method overloading: different call semantics possible
				 «ENDIF»
				 «appendJSDocSummaryAndWriteSeeAndDescription(operation, "* ")»
				 *
				 «writeJSDocForSignature(operation, "* ")»
				 */
			«ENDFOR»
			this.«operationName» = new settings.proxyElementTypes.ProxyOperation(this, settings, "«operationName»", [
				«FOR operation: getMethods(fInterface, operationName) SEPARATOR ","»
				{
					inputParameter: [
					«FOR param: getInputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : «param.typeNameForParameter»
						}
					«ENDFOR»
					],
					outputParameter: [
						«FOR param: getOutputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : «param.typeNameForParameter»
						}
						«ENDFOR»
					]
				}«ENDFOR»
			]).buildFunction();
	«ENDFOR»

	«FOR event: getEvents(fInterface)»
	«val eventName = event.joynrName»
	«val filterParameters = getFilterParameters(event)»
			/**
			 * @name «fInterface.proxyName»#«eventName»
			 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
			 */
			this.«eventName» = new settings.proxyElementTypes.ProxyEvent(this, {
					broadcastName : "«eventName»",
					broadcastParameter : [
						«FOR param: event.outputParameters SEPARATOR ", "»
						{
							name : "«param.joynrName»",
							type : «param.typeNameForParameter»
						}
						«ENDFOR»
					],
					messagingQos : settings.messagingQos,
					discoveryQos : settings.discoveryQos,
					«IF isSelective(event)»
					dependencies: {
							subscriptionManager: settings.dependencies.subscriptionManager
						},
					filterParameters: {
						«FOR filterParameter : filterParameters SEPARATOR ","»
							"«filterParameter»": "reservedForTypeInfo"
						«ENDFOR»
					}
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
				value: "«getFQN(fInterface)»"
			});
		};

		«fInterface.proxyName».getUsedDatatypes = function getUsedDatatypes(){
			return [
						«FOR datatype : fInterface.getAllComplexAndEnumTypes.filter[a | a instanceof FType] SEPARATOR ','»
						"«(datatype as FType).toTypesEnum»"
						«ENDFOR»
					];
		};

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«fInterface.defineName(fInterface.proxyName)»[
				«FOR datatype : fInterface.getAllComplexAndEnumTypes.filter[a | a instanceof FType] SEPARATOR ','»
						"«(datatype as FType).getDependencyPath»"
				«ENDFOR»
				], function () {
					return «fInterface.proxyName»;
				});
		} else if (typeof exports !== 'undefined' ) {
			if ((module !== undefined) && module.exports) {				
				«FOR datatype : getAllComplexAndEnumTypes(fInterface, false, true, true, true, true, true, false).filter[a | a instanceof FType]»
					require("«relativePathToBase() + (datatype as FType).getDependencyPath()»");
				«ENDFOR»
				exports = module.exports = «fInterface.proxyName»;
			}
			else {
				// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
				exports.«fInterface.proxyName» = «fInterface.proxyName»;
			}
		} else {
			window.«fInterface.proxyName» = «fInterface.proxyName»;
		}
		«ELSE»
		window.«fInterface.proxyName» = «fInterface.proxyName»;
		«ENDIF»
	})();
	'''

}
