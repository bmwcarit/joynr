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
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import java.util.ArrayList
import java.util.HashMap
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod

class DefaultInterfaceProviderTemplate implements InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil
	@Inject extension JavaTypeUtil
	@Inject extension TemplateBase
	@Inject extension InterfaceProviderTemplate

	override generate(FInterface serviceInterface) {
		var methodToDeferredName = new HashMap<FMethod, String>();
		var uniqueMethodsToCreateDeferreds = new ArrayList<FMethod>();
		init(serviceInterface, methodToDeferredName, uniqueMethodsToCreateDeferreds);

		val interfaceName =  serviceInterface.joynrName
		val className = "Default" + interfaceName + "Provider"
		val abstractProviderName = interfaceName + "AbstractProvider"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''
«warning()»
package «packagePath»;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

«FOR datatype: getRequiredIncludesFor(serviceInterface, true, true, true, false, false)»
	import «datatype»;
«ENDFOR»

public class «className» extends «abstractProviderName» {
	private static final Logger logger = LoggerFactory.getLogger(«className».class);

	«FOR attribute: getAttributes(serviceInterface)»
		«val attributeName = attribute.joynrName»
		«val attributeType = attribute.typeName»
		protected «attributeType» «attributeName»;
	«ENDFOR»

	public «className»() {
		// default uses a priority that is the current time,
		// causing arbitration to the last started instance
		providerQos.setPriority(System.currentTimeMillis());
	}

	«FOR attribute : getAttributes(serviceInterface)»
		«val attributeName = attribute.joynrName»
		«val attributeType = attribute.typeName»

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
		«var params = method.typedParameterListJavaRpc»
		«val outputParameters = getOutputParameters(method)»

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
			«FOR outputParameter : outputParameters»
				«outputParameter.typeName» «outputParameter.name» = «outputParameter.defaultValue»;
			«ENDFOR»
			deferred.resolve(«method.commaSeperatedUntypedOutputParameterList»);
			return new Promise<«deferredName»>(deferred);
		}
	«ENDFOR»
}
		'''
	}
}