package io.joynr.generator.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
import org.franca.core.franca.FMethod

class InterfaceProviderAsyncTemplate {
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "ProviderAsync"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''

		«warning()»
		package «packagePath»;

		«IF needsListImport(serviceInterface)»
		import java.util.List;
		«ENDIF»
		import com.fasterxml.jackson.core.type.TypeReference;
		import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
		«IF getMethods(serviceInterface).size > 0 || hasReadAttribute(serviceInterface)»
		import io.joynr.dispatcher.rpc.Callback;
		import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
		«ENDIF»
		«IF hasWriteAttribute(serviceInterface) || hasMethodWithArguments(serviceInterface)»
		import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
		«ENDIF»

		import io.joynr.provider.JoynrProviderAsync;

		«FOR datatype: getRequiredIncludesFor(serviceInterface, true, true, true, false)»
			import «datatype»;
		«ENDFOR»
		public interface «className» extends «interfaceName», JoynrProviderAsync, JoynrAsyncInterface {
			public static class VoidToken extends TypeReference<Void> {
			}
			«FOR attribute: getAttributes(serviceInterface)»
				«var attributeName = attribute.joynrName»
				«var attributeType = getObjectDataTypeForPlainType(getMappedDatatypeOrList(attribute))»
				«var getAttribute = "get" + attributeName.toFirstUpper»
				«var setAttribute = "set" + attributeName.toFirstUpper»
				«IF isReadable(attribute)»

					void «getAttribute»(@JoynrRpcCallback(deserialisationType = «getTokenTypeForArrayType(attributeType)»Token.class) Callback<«attributeType»> callback);
				«ENDIF»
				«IF isWritable(attribute)»

					void «setAttribute»(
							@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
							@JoynrRpcParam(value="«attributeName»", deserialisationType = «getTokenTypeForArrayType(attributeType)»Token.class) «attributeType» «attributeName»);
				«ENDIF»
			«ENDFOR»
			«FOR method: getMethods(serviceInterface)»
				«var methodName = method.joynrName»
				«var params = getTypedParameterListJavaRpc(method)»
				«var callbackParameter = getCallbackParameter(method)»

				/*
				* «methodName»
				*/
				public void «methodName»(
						«callbackParameter»«IF !params.equals("")»,«ENDIF»
						«IF !params.equals("")»«params»«ENDIF»
				);
			«ENDFOR»

		}

		'''
	}

	def getCallbackParameter(FMethod method) {
		var outPutParameterType = getMappedOutputParameter(method).iterator.next;
		var outPutObjectType = getObjectDataTypeForPlainType(outPutParameterType);
		if (outPutParameterType!="void"){
			if (outPutObjectType == ""){
				return "@JoynrRpcCallback(deserialisationType = "+getTokenTypeForArrayType(outPutParameterType)+"Token.class) Callback<"+outPutParameterType+"> callback"
			}
			else{
				return "@JoynrRpcCallback(deserialisationType = "+getTokenTypeForArrayType(outPutObjectType)+"Token.class) Callback<"+outPutParameterType+"> callback"
			}
		} else {
			return "@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback"
		}
	}

}
