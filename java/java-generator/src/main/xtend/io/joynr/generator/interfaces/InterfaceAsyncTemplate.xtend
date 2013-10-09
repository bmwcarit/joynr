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
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FInterface
import io.joynr.generator.util.JoynrJavaGeneratorExtensions

class InterfaceAsyncTemplate {
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase



	
	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.name.toFirstUpper
		val asyncClassName = interfaceName + "Async"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		

		'''
«warning()»
package «packagePath»;

«IF needsListImport(serviceInterface)»
import java.util.List;
«ENDIF»
import com.fasterxml.jackson.core.type.TypeReference;

import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.proxy.Future;

«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	import «datatype»;
«ENDFOR»
	
//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used. 
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@SuppressWarnings("unused")
public interface «asyncClassName» extends «interfaceName», JoynrAsyncInterface {

	public static class VoidToken extends TypeReference<Void> {}	
	«FOR attribute: getAttributes(serviceInterface)»
		«var attributeName = attribute.name.toFirstUpper»
		«var attributeType = getObjectDataTypeForPlainType(getMappedDatatypeOrList(attribute))» 
		«var getAttribute = "get" + attributeName» 
		«IF isReadable(attribute)»			
			public Future<«attributeType»> «getAttribute»(@JoynrRpcCallback(deserialisationType = «getTokenTypeForArrayType(attributeType)»Token.class) Callback<«attributeType»> callback);	
		«ENDIF»	
	«ENDFOR»	
	
	«FOR method: getMethods(serviceInterface)»
		«var methodName = method.name»
		«var outPutParameterType = getMappedOutputParameter(method).iterator.next»
		«var outPutObjectType = getObjectDataTypeForPlainType(outPutParameterType)»
		«var params = getTypedParameterListJavaRpc(method)»
		/*
		* «methodName»
		*/
		«IF getMappedOutputParameter(method).iterator.next=="void"»
		public void «methodName»(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback «generateMethodEnd(params)»
		«ELSE»
			«IF outPutObjectType == ""»
			public Future<«outPutParameterType»> «methodName»(@JoynrRpcCallback(deserialisationType = «getTokenTypeForArrayType(outPutParameterType)»Token.class) Callback<«outPutParameterType»> callback «generateMethodEnd(params)»
		   	«ELSE»
			public Future<«outPutObjectType»> «methodName»(@JoynrRpcCallback(deserialisationType = «getTokenTypeForArrayType(outPutObjectType)»Token.class) Callback<«outPutObjectType»> callback «generateMethodEnd(params)»
			«ENDIF»
		«ENDIF»
	«ENDFOR»
}
		'''	
	}	
	
	def generateMethodEnd(String params) {
		'''
		«IF params.equals("")»
			);
		«ELSE»
			, «params»);	
		«ENDIF»
		'''
	}			
}
