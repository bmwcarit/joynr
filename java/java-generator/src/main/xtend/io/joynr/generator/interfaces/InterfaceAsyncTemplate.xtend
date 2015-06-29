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
import org.franca.core.franca.FMethod
import io.joynr.generator.util.InterfaceTemplate
import java.util.ArrayList
import java.util.HashMap

class InterfaceAsyncTemplate implements InterfaceTemplate{
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase
	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToCallbackName, HashMap<FMethod, String> methodToFutureName, HashMap<FMethod, String> methodToSyncReturnedName, ArrayList<FMethod> uniqueMultioutMethods) {
		var uniqueMultioutMethodSignatureToOutputContainerName = new HashMap<String, String>();
		var methodCounts = overloadedMethodCounts(getMethods(serviceInterface));
		var indexForMethod = new HashMap<String, Integer>();

		for (FMethod method : getMethods(serviceInterface)) {
			if (method.outputParameters.size < 2) {
				val outputParamterType = getMappedOutputParameter(method).iterator.next;
				if (outputParamterType == "void") {
					methodToCallbackName.put(method, "Callback<Void>");
					methodToFutureName.put(method, "Future<Void>");
					methodToSyncReturnedName.put(method, "Void");
				} else {
					methodToCallbackName.put(method, "Callback<" + getObjectDataTypeForPlainType(outputParamterType) + ">");
					methodToFutureName.put(method, "Future<" + getObjectDataTypeForPlainType(outputParamterType) + ">");
					methodToSyncReturnedName.put(method, getObjectDataTypeForPlainType(outputParamterType));
				}
			} else {
				// Multiple Out Parameters
				var callbackName = method.name.toFirstUpper;
				var futureName = method.name.toFirstUpper;
				var syncReturnedName = method.name.toFirstUpper;
				if (methodCounts.get(method.name) == 1) {
				// method not overloaded, so no index needed
					uniqueMultioutMethods.add(method);
					callbackName += "Callback";
					futureName += "Future";
					syncReturnedName += "Returned";
				} else {
					// initialize index if not existent
					if (!indexForMethod.containsKey(method.name)) {
						indexForMethod.put(method.name, 0);
					}
					val methodSignature = createMethodSignature(method);
					if (!uniqueMultioutMethodSignatureToOutputContainerName.containsKey(methodSignature)) {
						var Integer index = indexForMethod.get(method.name);
						index++;
						indexForMethod.put(method.name, index);
						uniqueMultioutMethodSignatureToOutputContainerName.put(methodSignature, method.name.toFirstUpper + index);
						uniqueMultioutMethods.add(method);
					}
					val outputContainerName = uniqueMultioutMethodSignatureToOutputContainerName.get(methodSignature);
					callbackName +=  outputContainerName+ "Callback";
					futureName += outputContainerName + "Future";
					syncReturnedName += outputContainerName + "Returned";
				}
				methodToCallbackName.put(method, callbackName);
				methodToFutureName.put(method, futureName);
				methodToSyncReturnedName.put(method, syncReturnedName);
			}
		}
	}

	override generate(FInterface serviceInterface) {
		var methodToCallbackName = new HashMap<FMethod, String>();
		var methodToFutureName = new HashMap<FMethod, String>();
		var methodToSyncReturnedName = new HashMap<FMethod, String>();
		var uniqueMultioutMethods = new ArrayList<FMethod>();
		init(serviceInterface, methodToCallbackName, methodToFutureName, methodToSyncReturnedName, uniqueMultioutMethods);
		val interfaceName =  serviceInterface.joynrName
		val asyncClassName = interfaceName + "Async"

		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		val hasReadAttribute = hasReadAttribute(serviceInterface);
		val hasMethodWithArguments = hasMethodWithArguments(serviceInterface);
		val hasWriteAttribute = hasWriteAttribute(serviceInterface);
		'''
«warning()»
package «packagePath»;

«IF needsListImport(serviceInterface)»
import java.util.List;
«ENDIF»
import com.fasterxml.jackson.core.type.TypeReference;
import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
«IF getMethods(serviceInterface).size > 0 || hasReadAttribute»
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
«ENDIF»
«IF hasWriteAttribute || hasMethodWithArguments»
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
«ENDIF»
«IF uniqueMultioutMethods.size > 0»
import io.joynr.proxy.ICallback;
import io.joynr.exceptions.JoynrRuntimeException;
«ENDIF»
«IF hasWriteAttribute»
import io.joynr.exceptions.JoynrArbitrationException;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(serviceInterface, true, true, true, false, false)»
	import «datatype»;
«ENDFOR»

«FOR method: uniqueMultioutMethods»
«val syncReturnedName = methodToSyncReturnedName.get(method)»
	import «packagePath».«interfaceName»Sync.«syncReturnedName»;
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

	«FOR method: uniqueMultioutMethods»
	«val futureName = methodToFutureName.get(method)»
	«val syncReturnedName = methodToSyncReturnedName.get(method)»
	public static class «futureName» extends Future<«syncReturnedName»> {
		public void resolve(Object... outParameters) {
			if (outParameters[0] instanceof JoynrRuntimeException) {
				onFailure((JoynrRuntimeException) outParameters[0]);
			} else {
				onSuccess(new «syncReturnedName»(outParameters));
			}
		}
	}
	«ENDFOR»

	«FOR method: uniqueMultioutMethods»
	«val callbackName = methodToCallbackName.get(method)»
	public abstract class «callbackName» implements ICallback {
		public abstract void onSuccess(«getTypedOutputParametersCommaSeparated(method)»);

		public void resolve(Object... outParameters) {
			if (outParameters[0] instanceof JoynrRuntimeException) {
				onFailure((JoynrRuntimeException) outParameters[0]);
			} else {
					«var index = 0»
					onSuccess(
						«FOR outParameter : method.outputParameters»
							«IF isEnum(outParameter.type)»
								«getMappedDatatypeOrList(outParameter)».valueOf((String) outParameters[«index++»])«IF index < method.outputParameters.length»,«ENDIF»
							«ELSE»
								(«getMappedDatatypeOrList(outParameter)») outParameters[«index++»]«IF index < method.outputParameters.length»,«ENDIF»
							«ENDIF»
						«ENDFOR»
				);
			}
		}
	}
	«ENDFOR»

	«FOR method: getMethods(serviceInterface)»
		«var methodName = method.joynrName»
		«var params = getTypedParameterListJavaRpc(method)»
		«var callbackParameter = getCallbackParameter(method, methodToCallbackName)»

		/*
		* «methodName»
		*/
		public «methodToFutureName.get(method)» «methodName»(
				«callbackParameter»«IF !params.equals("")»,«ENDIF»
				«IF !params.equals("")»«params»«ENDIF»
		);
	«ENDFOR»
}
		'''
	}

	def getCallbackParameter(FMethod method, HashMap<FMethod, String> methodToCallbackName) {
		var outPutParameterType = getMappedOutputParameter(method).iterator.next;
		var callbackType = methodToCallbackName.get(method);
		var outPutObjectType = getObjectDataTypeForPlainType(outPutParameterType);
		if (method.outputParameters.size < 2) {
			if (outPutParameterType!="void"){
				if (outPutObjectType == ""){
					return "@JoynrRpcCallback(deserialisationType = "+getTokenTypeForArrayType(outPutParameterType)+"Token.class) "+ callbackType + " callback"
				}
				else{
					return "@JoynrRpcCallback(deserialisationType = "+getTokenTypeForArrayType(outPutObjectType)+"Token.class) " + callbackType + " callback"
				}
			} else {
				return "@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback"
			}
		} else {
			return "@JoynrRpcCallback " + callbackType + " callback"
		}
	}
}
