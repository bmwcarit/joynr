package io.joynr.generator.provider
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
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FInterface
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate
import org.franca.core.franca.FMethod
import java.util.HashMap
import java.util.ArrayList

class DefaultInterfaceProviderTemplate implements InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension InterfaceProviderAsyncTemplate

	override generate(FInterface serviceInterface) {
		var methodToDeferredName = new HashMap<FMethod, String>();
		var uniqueMethodsToCreateDeferreds = new ArrayList<FMethod>();
		init(serviceInterface, methodToDeferredName, uniqueMethodsToCreateDeferreds);

		val interfaceName =  serviceInterface.joynrName
		val className = "Default" + interfaceName + "ProviderAsync"
		val abstractProviderName = interfaceName + "AbstractProviderAsync"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''
«warning()»
package «packagePath»;
«IF needsListImport(serviceInterface)»
	import java.util.ArrayList;
	import java.util.List;
«ENDIF»
import com.google.inject.Singleton;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import io.joynr.provider.PromiseListener;
import io.joynr.exceptions.JoynrException;
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;

import io.joynr.provider.Promise;
«IF hasReadAttribute(serviceInterface)»
	import io.joynr.provider.Deferred;
«ENDIF»
«IF hasWriteAttribute(serviceInterface) || hasMethodWithArguments(serviceInterface)»
	import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
«ENDIF»
«IF hasWriteAttribute(serviceInterface) || hasMethodWithoutReturnValue(serviceInterface)»
	import io.joynr.provider.DeferredVoid;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	import «datatype»;
«ENDFOR»
import «packagePath».«interfaceName».*;

@Singleton
public class «className» extends «abstractProviderName» {
	private static final Logger logger = LoggerFactory.getLogger(«className».class);

	«FOR attribute: getAttributes(serviceInterface)»
		«val attributeName = attribute.joynrName»
		«val attributeType = getMappedDatatypeOrList(attribute)»
		protected «attributeType» «attributeName»;
	«ENDFOR»

	public «className»() {
		// default uses a priority that is the current time,
		// causing arbitration to the last started instance
		providerQos.setPriority(System.currentTimeMillis());
	}

	«FOR attribute : getAttributes(serviceInterface)»
		«val attributeName = attribute.joynrName»
		«val attributeType = getMappedDatatypeOrList(attribute)»

		«IF isReadable(attribute)»
			@Override
			public Promise<Deferred<«attributeType»>> get«attributeName.toFirstUpper»() {
				Deferred<«attributeType»> deferred = new Deferred<«attributeType»>();
				deferred.resolve(«attributeName»);
				return new Promise<Deferred<«attributeType»>>(deferred);
			}
		«ENDIF»
		«IF isWritable(attribute)»
			@Override
			public Promise<DeferredVoid> set«attributeName.toFirstUpper»(«attributeType» «attributeName») {
				DeferredVoid deferred = new DeferredVoid();
				this.«attributeName» = «attributeName»;
				«IF isNotifiable(attribute)»
					«attributeName»Changed(«attributeName»);
				«ENDIF»
				deferred.resolve();
				return new Promise<DeferredVoid>(deferred);
			}
		«ENDIF»
	«ENDFOR»

	«FOR method : getMethods(serviceInterface)»
		«var methodName = method.joynrName»
		«var deferredName = methodToDeferredName.get(method)»
		«var params = getTypedParameterListJavaRpc(method)»
		«val outputParameters = getOutputParameters(method)»
		«val outputParameterType = mapOutputParameters(outputParameters).iterator.next»
		«val outputParameter = if (!outputParameters.isEmpty) outputParameters.iterator.next else null»
		/*
		* «methodName»
		*/
		@Override
		public Promise<«deferredName»> «methodName»(
				«IF !params.equals("")»«params»«ENDIF») {
			logger.warn("**********************************************");
			logger.warn("* «className».«methodName» called");
			logger.warn("**********************************************");
			«deferredName» deferred = new «deferredName»();

			«IF outputParameterType=="void"»
				deferred.resolve();
			«ELSEIF outputParameterType=="String"»
				deferred.resolve("Hello World");
			«ELSEIF outputParameterType=="Boolean"»
				deferred.resolve(false);
			«ELSEIF outputParameterType=="Integer"»
				deferred.resolve(42);
			«ELSEIF outputParameterType=="Double"»
				deferred.resolve(3.1415);
			«ELSEIF outputParameterType=="Long"»
				deferred.resolve((long) 42);
			«ELSEIF outputParameterType=="Byte"»
				deferred.resolve((byte) 42);
			«ELSEIF outputParameterType.startsWith("List<")»
				deferred.resolve(new Array«outputParameterType»());
			«ELSEIF isEnum(outputParameter.type)»
				deferred.resolve(«outputParameterType».«getEnumElements(getEnumType(outputParameter.type)).iterator.next.joynrName»);
			«ELSE»
				deferred.resolve(new «outputParameterType»());
			«ENDIF»
			return new Promise<«deferredName»>(deferred);
		}

		// TODO: remove begin
		«var callbackParameter = getCallbackParameter(method)»
		public void «methodName»(
				final «callbackParameter»«IF !params.equals("")»,«ENDIF»
				«IF !params.equals("")»«params»«ENDIF»
		) {
			«methodName»(«getCommaSeperatedUntypedParameterList(method)»).then(new PromiseListener() {
				@Override
				public void onRejection(JoynrException error) {
					callback.onFailure(error);
				}
				«IF !outputParameters.isEmpty && isArray(outputParameter)»
				@SuppressWarnings("unchecked")
				«ENDIF»
				@Override
				public void onFulfillment(Object... values) {
					«IF !outputParameters.isEmpty»
						callback.onSuccess((«outputParameterType») values[0]);
					«ELSE»
						callback.onSuccess(null);
					«ENDIF»
				}
			});
		}
		// TODO: remove end
	«ENDFOR»
}
		'''
	}
}