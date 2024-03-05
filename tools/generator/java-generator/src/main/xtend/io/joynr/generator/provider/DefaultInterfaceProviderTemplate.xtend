package io.joynr.generator.provider
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
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import java.util.ArrayList
import java.util.HashMap
import org.franca.core.franca.FMethod

class DefaultInterfaceProviderTemplate extends InterfaceProviderTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil
	@Inject extension AttributeUtil
	@Inject extension MethodUtil
	@Inject extension JavaTypeUtil
	@Inject extension TemplateBase

	override generate(boolean generateVersion) {
		var methodToDeferredName = new HashMap<FMethod, String>();
		var uniqueMethodsToCreateDeferreds = new ArrayList<FMethod>();
		init(francaIntf, methodToDeferredName, uniqueMethodsToCreateDeferreds);

		val interfaceName =  francaIntf.joynrName
		val className = "Default" + interfaceName + "Provider"
		val abstractProviderName = interfaceName + "AbstractProvider"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".", generateVersion)

		'''
«warning()»
package «packagePath»;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

«IF hasNonFireAndForgetMethods(francaIntf) || hasReadAttribute(francaIntf) || hasWriteAttribute(francaIntf)»
	import io.joynr.provider.Promise;
	«IF hasReadAttribute(francaIntf)»
		import io.joynr.provider.Deferred;
	«ENDIF»
	«IF hasWriteAttribute(francaIntf) || hasMethodWithoutReturnValue(francaIntf)»
		import io.joynr.provider.DeferredVoid;
	«ENDIF»
«ENDIF»

«FOR datatype: getRequiredIncludesFor(francaIntf, true, true, true, false, false, true, generateVersion)»
	import «datatype»;
«ENDFOR»

public class «className» extends «abstractProviderName» {
	private static final Logger logger = LoggerFactory.getLogger(«className».class);

	«FOR attribute: getAttributes(francaIntf)»
		«val attributeName = attribute.joynrName»
		«IF attribute.type.isTypeDef»
			«val typeDefType = attribute.type.typeDefType.actualType.typeName»
			// type resolved from modeled Franca typedef «attribute.type.joynrName» as «typeDefType»
		«ENDIF»
		protected «attribute.typeName» «attributeName»;
	«ENDFOR»

	public «className»() {
	}

	«FOR attribute : getAttributes(francaIntf)»
		«val attributeName = attribute.joynrName»
		«val attributeType = attribute.typeName»

		«IF isReadable(attribute)»
			@Override
			public Promise<Deferred<«attributeType»>> get«attributeName.toFirstUpper»() {
				Deferred<«attributeType»> deferred = new Deferred<>();
				deferred.resolve(«attributeName»);
				return new Promise<>(deferred);
			}
		«ENDIF»

		«IF isWritable(attribute)»
			@Override
			public Promise<DeferredVoid> set«attributeName.toFirstUpper»(«attributeType» «attributeName») {
				DeferredVoid deferred = new DeferredVoid();
				«IF (isArray(attribute) || isByteBuffer(attribute.type))»
				if («attributeName» != null) {
					this.«attributeName» = «attributeName».clone();
				} else {
					this.«attributeName» = null;
				}
				«ELSEIF (isMap(attribute.type) || isCompound(attribute.type))»
				this.«attributeName» = new «attributeType»(«attributeName»);
				«ELSE»
				this.«attributeName» = «attributeName»;
				«ENDIF»
				«IF isNotifiable(attribute)»
				«attributeName»Changed(«attributeName»);
				«ENDIF»
				deferred.resolve();
				return new Promise<>(deferred);
			}
		«ENDIF»
	«ENDFOR»

	«FOR method : getMethods(francaIntf)»
		«var methodName = method.joynrName»
		«var deferredName = methodToDeferredName.get(method)»
		«var params = method.inputParameters.typedParameterList»
		«val outputParameters = getOutputParameters(method)»

		/*
		* «methodName»
		*/
		@Override
		«IF method.fireAndForget»
		public void «methodName»(
		«ELSE»
		public Promise<«deferredName»> «methodName»(
		«ENDIF»
				«IF !params.equals("")»«params»«ENDIF») {
			logger.warn("**********************************************");
			logger.warn("* «className».«methodName» called");
			logger.warn("**********************************************");
			«IF !method.fireAndForget»
			«deferredName» deferred = new «deferredName»();
			«FOR outputParameter : outputParameters»
				«outputParameter.typeName» «outputParameter.name» = «outputParameter.defaultValue»;
			«ENDFOR»
			deferred.resolve(«method.commaSeperatedUntypedOutputParameterList»);
			return new Promise<>(deferred);
			«ENDIF»
		}
	«ENDFOR»
}
		'''
	}
}
