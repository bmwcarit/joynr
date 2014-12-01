package io.joynr.generator.interfaces
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod

class InterfaceAsyncTemplate {
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val asyncClassName = interfaceName + "Async"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		val hasReadAttribute = hasReadAttribute(serviceInterface);
		val hasMethodWithArguments = hasMethodWithArguments(serviceInterface);
		val hasWriteAttribute = hasWriteAttribute(serviceInterface);
		val hasMethodWithReturnValue = hasMethodWithReturnValue(serviceInterface);
		'''
«warning()»
package «packagePath»;

«IF needsListImport(serviceInterface)»
import java.util.List;
«ENDIF»
import com.fasterxml.jackson.core.type.TypeReference;
import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
«IF getMethods(serviceInterface).size > 0 || hasReadAttribute»
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
«ENDIF»
«IF hasWriteAttribute || hasMethodWithArguments»
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
«ENDIF»
«IF hasMethodWithReturnValue || hasReadAttribute || hasWriteAttribute»
import io.joynr.proxy.Future;
«ENDIF»
«IF hasWriteAttribute»
import io.joynr.exceptions.JoynrArbitrationException;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(serviceInterface, true, true, true, false)»
	import «datatype»;
«ENDFOR»
	
public interface «asyncClassName» extends «interfaceName», JoynrAsyncInterface {

	public static class VoidToken extends TypeReference<Void> {

	}
	«FOR attribute: getAttributes(serviceInterface)»
		«var attributeName = attribute.joynrName»
		«var attributeType = getObjectDataTypeForPlainType(getMappedDatatypeOrList(attribute))» 
		«var getAttribute = "get" + attributeName.toFirstUpper»
		«var setAttribute = "set" + attributeName.toFirstUpper»
		«IF isReadable(attribute)»			

			public Future<«attributeType»> «getAttribute»(@JoynrRpcCallback(deserialisationType = «getTokenTypeForArrayType(attributeType)»Token.class) Callback<«attributeType»> callback);
		«ENDIF»
		«IF isWritable(attribute)»

			Future<Void> «setAttribute»(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback, @JoynrRpcParam(value="«attributeName»", deserialisationType = «getTokenTypeForArrayType(attributeType)»Token.class) «attributeType» «attributeName») throws JoynrArbitrationException;
		«ENDIF»	
	«ENDFOR»
	«FOR method: getMethods(serviceInterface)»
		«var methodName = method.joynrName»
		«var params = getTypedParameterListJavaRpc(method)»
		«var callbackParameter = getCallbackParameter(method)»

		/*
		* «methodName»
		*/
		public «getFutureObject(method)» «methodName»(
				«callbackParameter»«IF !params.equals("")»,«ENDIF»
				«IF !params.equals("")»«params»«ENDIF»
		);
	«ENDFOR»
}
		'''	
	}

	def getFutureObject(FMethod method) {
		var outPutParameterType = getMappedOutputParameter(method).iterator.next;
		var outPutObjectType = getObjectDataTypeForPlainType(outPutParameterType);
		if (outPutParameterType!="void"){
			if (outPutObjectType == ""){
				return "Future<" + outPutParameterType + ">"
			}
			else{
				return "Future<" + outPutObjectType + ">"
			}
		} else {
			return "void"//"Future<Void>"
		}
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
