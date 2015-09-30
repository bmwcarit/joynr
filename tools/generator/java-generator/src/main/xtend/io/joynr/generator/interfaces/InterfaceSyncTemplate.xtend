package io.joynr.generator.interfaces
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
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FInterface
import io.joynr.generator.util.InterfaceTemplate
import java.util.HashMap
import org.franca.core.franca.FMethod
import java.util.ArrayList
import io.joynr.generator.util.JavaTypeUtil

class InterfaceSyncTemplate implements InterfaceTemplate{
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension TemplateBase
	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToReturnTypeName, ArrayList<FMethod> uniqueMultioutMethods) {
		var uniqueMultioutMethodSignatureToContainerNames = new HashMap<String, String>();
		var methodCounts = overloadedMethodCounts(getMethods(serviceInterface));
		var indexForMethod = new HashMap<String, Integer>();

		for (FMethod method : getMethods(serviceInterface)) {
			if (method.outputParameters.size < 2) {
				val outputParamterType = method.typeNamesForOutputParameter.iterator.next;
				if (outputParamterType == "void") {
					methodToReturnTypeName.put(method, "void");
				} else {
					methodToReturnTypeName.put(method, outputParamterType.objectDataTypeForPlainType);
				}
			} else {
				// Multiple Out Parameters
				var containerName = method.name.toFirstUpper;
				if (methodCounts.get(method.name) == 1) {
				// method not overloaded, so no index needed
					uniqueMultioutMethods.add(method);
					containerName += "Returned";
				} else {
					// initialize index if not existent
					if (!indexForMethod.containsKey(method.name)) {
						indexForMethod.put(method.name, 0);
					}
					val methodSignature = method.createMethodSignatureFromOutParameters;
					if (!uniqueMultioutMethodSignatureToContainerNames.containsKey(methodSignature)) {
						var Integer index = indexForMethod.get(method.name);
						index++;
						indexForMethod.put(method.name, index);
						uniqueMultioutMethodSignatureToContainerNames.put(methodSignature, method.name.toFirstUpper + index);
						uniqueMultioutMethods.add(method);
					}
					containerName += uniqueMultioutMethodSignatureToContainerNames.get(methodSignature) + "Returned";
				}
				methodToReturnTypeName.put(method, containerName);
			}
		}
	}

	override generate(FInterface serviceInterface) {
		var methodToReturnTypeName = new HashMap<FMethod, String>();
		var uniqueMultioutMethods = new ArrayList<FMethod>();
		init(serviceInterface, methodToReturnTypeName, uniqueMultioutMethods);
		val interfaceName =  serviceInterface.joynrName
		val syncClassName = interfaceName + "Sync"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		val hasMethodWithArguments = hasMethodWithArguments(serviceInterface);
		val hasWriteAttribute = hasWriteAttribute(serviceInterface);
		val hasMethodWithReturnValue = hasMethodWithReturnValue(serviceInterface);
		'''
«warning()»

package «packagePath»;

«IF needsListImport(serviceInterface)»
	import java.util.List;
«ENDIF»

import io.joynr.dispatcher.rpc.JoynrSyncInterface;
«IF hasReadAttribute(serviceInterface) || hasMethodWithReturnValue»
	import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;
«ENDIF»
«IF hasWriteAttribute || hasMethodWithArguments»
	import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
«ENDIF»

import io.joynr.exceptions.JoynrRuntimeException;
«IF hasMethodWithErrorEnum(serviceInterface)»
	import joynr.exceptions.ApplicationException;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(serviceInterface, true, true, true, false, false)»
	import «datatype»;
«ENDFOR»

public interface «syncClassName» extends «interfaceName», JoynrSyncInterface {

«FOR attribute: getAttributes(serviceInterface) SEPARATOR "\n"»
	«var attributeName = attribute.joynrName»
	«var attributeType = attribute.typeName.objectDataTypeForPlainType»
	«var getAttribute = "get" + attributeName.toFirstUpper»
	«var setAttribute = "set" + attributeName.toFirstUpper»
		«IF isReadable(attribute)»
			@JoynrRpcReturn(deserializationType = «getTokenTypeForArrayType(attributeType)»Token.class)
			public «attributeType» «getAttribute»() throws JoynrRuntimeException;
		«ENDIF»
		«IF isWritable(attribute)»
			void «setAttribute»(@JoynrRpcParam(value="«attributeName»", deserializationType = «getTokenTypeForArrayType(attributeType)»Token.class) «attributeType» «attributeName») throws JoynrRuntimeException;
		«ENDIF»
«ENDFOR»

«FOR method: uniqueMultioutMethods»
	«val containerName = methodToReturnTypeName.get(method)»
		public class «containerName» {
			«FOR outParameter : method.outputParameters»
				public final «outParameter.typeName» «outParameter.name»;
			«ENDFOR»
			public «containerName»(Object... outParameters) {
				«var index = 0»
				«FOR outParameter : method.outputParameters»
					«IF isEnum(outParameter.type)»
					this.«outParameter.name» = «outParameter.typeName».valueOf((String) outParameters[«index++»]);
					«ELSE»
					this.«outParameter.name» = («outParameter.typeName») outParameters[«index++»];
					«ENDIF»
				«ENDFOR»
			}
		}
«ENDFOR»

«FOR method: getMethods(serviceInterface) SEPARATOR "\n"»
	«var methodName = method.joynrName»
	«var outputParameters = method.typeNamesForOutputParameter»
		/*
		* «methodName»
		*/
		«IF outputParameters.size > 1»
			public «methodToReturnTypeName.get(method)» «methodName»(
					«method.typedParameterListJavaRpc»
		«ELSE»
			«IF method.typeNamesForOutputParameter.iterator.next=="void"»
				public «methodToReturnTypeName.get(method)» «methodName»(
						«getTypedParameterListJavaRpc(method)»
			«ELSE»
				@JoynrRpcReturn(deserializationType = «getTokenTypeForArrayType(method.typeNamesForOutputParameter.iterator.next)»Token.class)
				public «methodToReturnTypeName.get(method)» «methodName»(
						«getTypedParameterListJavaRpc(method)»
			«ENDIF»
		«ENDIF»
		) throws JoynrRuntimeException«IF method.hasErrorEnum», ApplicationException«ENDIF»;
«ENDFOR»
}
		'''
	}

}
