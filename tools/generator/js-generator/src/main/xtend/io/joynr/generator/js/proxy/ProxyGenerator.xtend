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
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.util.Date
import org.eclipse.xtext.generator.IFileSystemAccess

class ProxyGenerator extends InterfaceJsTemplate {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension JSTypeUtil
	@Inject extension NamingUtil
	@Inject extension MethodUtil
	@Inject extension BroadcastUtil
	@Inject extension InterfaceUtil

	def relativePathToBase() {
		var relativePath = ""
		for (var i=0; i<packagePathDepth; i++) {
			relativePath += "../"
		}
		return relativePath
	}

	def generateProxy(IFileSystemAccess fsa, boolean generateVersion){
		super.generate(generateVersion)
		var fileName = path + "" + proxyName + ".ts"
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

	def proxyName(){
		francaIntf.joynrName + "Proxy"
	}

	override generate(boolean generateVersion)'''
	«val generationDate = (new Date()).toString»
	«val attributes = getAttributes(francaIntf)»
	«val methodNames = getMethodNames(francaIntf)»
	«val events = getEvents(francaIntf)»

	«IF attributes.length > 0»
	import {«FOR attributeType: attributes.proxyAttributeNames SEPARATOR ','»«attributeType» «ENDFOR»} from "joynr/joynr/proxy/ProxyAttribute";
	«ENDIF»
	«IF methodNames.length > 0»
	import ProxyOperation = require("joynr/joynr/proxy/ProxyOperation");
	«ENDIF»
	«IF events.length > 0»
	import ProxyEvent = require("joynr/joynr/proxy/ProxyEvent");
	«ENDIF»

	import MessagingQos = require("joynr/joynr/messaging/MessagingQos");
	import JoynrDiscoveryQos = require("joynr/joynr/proxy/DiscoveryQos");

	«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes)»
		import «datatype.joynrName» = require("«relativePathToBase() + datatype.getDependencyPath(generateVersion)»");
	«ENDFOR»

	namespace «proxyName» {
		export interface ProxySettings {
			domain: string;
			joynrName: string;
			discoveryQos: JoynrDiscoveryQos;
			messagingQos: MessagingQos | MessagingQos.Settings;
			dependencies: any;
			proxyParticipantId: string;
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
			»(settings: «operationName.toFirstUpper»Args«i») => Promise<«operationName.toFirstUpper»Returns«i»>«
			ENDFOR»
		«ENDFOR»
	}

	/**
	 * @classdesc
	 * <br/>Generation date: «generationDate»
	 * <br/><br/>
	 * «proxyName», generated from the corresponding interface description.
	 «appendJSDocSummaryAndWriteSeeAndDescription(francaIntf, "* ")»
	 */
	class «proxyName» {

		public interfaceName: string = "«francaIntf.fullyQualifiedName»";
		public settings: «proxyName».ProxySettings;
		public domain: string;
		public messagingQos: MessagingQos;

		/** set by ProxyBuilder after Arbitration **/
		public providerDiscoveryEntry!: { participantId: string };
		public proxyParticipantId: string;

	«FOR attribute : attributes»
	«val attributeName = attribute.joynrName»
		/**
		 * @name «proxyName»#«attributeName»
		 * @summary The «attributeName» attribute is GENERATED FROM THE INTERFACE DESCRIPTION
		 «appendJSDocSummaryAndWriteSeeAndDescription(attribute, "* ")»
		 */
		public «attributeName»: «attribute.proxyAttributeName»<«attribute.tsTypeName»>;
	«ENDFOR»

	«FOR operationName : methodNames»
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
		public «operationName»: «proxyName».«operationName.toFirstUpper»Signature;
	«ENDFOR»

	«FOR event: events»
	«val eventName = event.joynrName»
	/**
	 * @name «proxyName»#«eventName»
	 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
	 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
	 */
	public «eventName»: ProxyEvent;
	«ENDFOR»

		/**
		 * @param settings the settings object for this function call
		 * @param settings.domain the domain name
		 * @param settings.joynrName the interface name //TODO: check do we need this?
		 *
		 * @param settings.discoveryQos the Quality of Service parameters for arbitration
		 * @param settings.discoveryQos.discoveryTimeoutMs for rpc calls to wait for arbitration to finish.
		 * @param settings.discoveryQos.arbitrationStrategy Strategy for choosing the appropriate provider from the list returned by the capabilities directory
		 * @param settings.discoveryQos.cacheMaxAgeMs Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
		 * @param settings.discoveryQos.discoveryScope If localOnly is set to true, only local providers will be considered.
		 * @param settings.discoveryQos.additionalParameters a map holding additional parameters in the form of key value pairs in the javascript object, e.g.: {"myKey": "myValue", "myKey2": 5}
		 *
		 * @param settings.messagingQos the Quality of Service parameters for messaging
		 * @param settings.messagingQos.ttl Roundtrip timeout for rpc requests.
		 * @param settings.dependencies instances of the internal objects needed by the proxy to interface with joynr
		 * @param settings.proxyParticipantId unique identifier of the proxy
		 */
		public constructor(settings: «proxyName».ProxySettings){
			this.domain = settings.domain;
			this.messagingQos = new MessagingQos(settings.messagingQos);
			this.proxyParticipantId = settings.proxyParticipantId;
			this.settings = settings;
			«FOR attribute : attributes»
			«val attributeName = attribute.joynrName»
			this.«attributeName» = new «attribute.proxyAttributeName»<«attribute.tsTypeName»>(this, "«attributeName»", "«attribute.getJoynrTypeName(generateVersion)»");
			«ENDFOR»
			«FOR operationName : methodNames»
			this.«operationName» = new ProxyOperation(this, "«operationName»", [
				«FOR operation: getMethods(francaIntf, operationName) SEPARATOR ","»
				{
					inputParameter: [
					«FOR param: getInputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : "«param.getJoynrTypeName(generateVersion)»"
						}
					«ENDFOR»
					],
					outputParameter: [
						«FOR param: getOutputParameters(operation) SEPARATOR ","»
						{
							name : "«param.joynrName»",
							type : "«param.getJoynrTypeName(generateVersion)»"
						}
						«ENDFOR»
					],
					fireAndForget: «IF operation.fireAndForget»true«ELSE»false«ENDIF»
				}«ENDFOR»
			]).buildFunction() as any;
			«ENDFOR»

			«FOR event: events»
			«val eventName = event.joynrName»
			«val filterParameters = getFilterParameters(event)»
			/**
			 * @name «proxyName»#«eventName»
			 * @summary The «eventName» event is GENERATED FROM THE INTERFACE DESCRIPTION
			 «appendJSDocSummaryAndWriteSeeAndDescription(event, "* ")»
			 */
			this.«eventName» = new ProxyEvent(this, {
				broadcastName : "«eventName»",
				broadcastParameter : [
					«FOR param: event.outputParameters SEPARATOR ", "»
					{
						name : "«param.joynrName»",
						type : "«param.getJoynrTypeName(generateVersion)»"
					}
					«ENDFOR»
				],
				messagingQos : this.messagingQos,
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
		}

		/**
		 * The MAJOR_VERSION of the proxy is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MAJOR_VERSION = «majorVersion»;

		/**
		 * The MINOR_VERSION of the proxy is GENERATED FROM THE INTERFACE DESCRIPTION
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

	export = «proxyName»;
	'''

}
