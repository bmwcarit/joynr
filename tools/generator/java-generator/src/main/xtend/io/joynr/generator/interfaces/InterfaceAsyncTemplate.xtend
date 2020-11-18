package io.joynr.generator.interfaces
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

class InterfaceAsyncTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension InterfaceUtil
	@Inject extension MethodUtil
	@Inject extension NamingUtil
	@Inject extension AttributeUtil
	@Inject extension TemplateBase

	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToCallbackName, HashMap<FMethod, String> methodToFutureName,  HashMap<FMethod, String> methodToErrorEnumName, HashMap<FMethod, String> methodToSyncReturnedName, ArrayList<FMethod> uniqueMultioutMethods, boolean generateVersion) {
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".", generateVersion)
		var uniqueMultioutMethodSignatureToOutputContainerName = new HashMap<String, String>();
		var methodCounts = overloadedMethodCounts(getMethods(serviceInterface));
		var indexForMethod = new HashMap<String, Integer>();

		for (FMethod method : getMethods(serviceInterface)) {
			if (method.outputParameters.size < 2) {
				val outputParamterType = method.typeNamesForOutputParameter.iterator.next;
				var outputType = if (outputParamterType == "void") "Void" else getObjectDataTypeForPlainType(outputParamterType)

				if (method.hasErrorEnum) {
					var errorEnumType = ""
					if (method.errors !== null) {
						errorEnumType = packagePath + "." + serviceInterface.joynrName + "." +
							methodToErrorEnumName.get(method)
					} else {
						errorEnumType = method.errorEnum.buildPackagePath(".", true, generateVersion) + "." + method.errorEnum.joynrName
					}
					methodToCallbackName.put(method, "CallbackWithModeledError<" + outputType + "," + errorEnumType + ">");
				} else {
					methodToCallbackName.put(method, "Callback<" + outputType + ">");
				}
				methodToFutureName.put(method, "Future<" + outputType + ">");
				methodToSyncReturnedName.put(method, outputType);
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
					val methodSignature = method.createMethodSignatureFromOutParameters;
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

	override generate(boolean generateVersion) {
		var methodToCallbackName = new HashMap<FMethod, String>();
		var methodToFutureName = new HashMap<FMethod, String>();
		var methodToErrorEnumName = francaIntf.methodToErrorEnumName
		var methodToSyncReturnedName = new HashMap<FMethod, String>();
		var uniqueMultioutMethods = new ArrayList<FMethod>();
		init(francaIntf, methodToCallbackName, methodToFutureName, methodToErrorEnumName, methodToSyncReturnedName, uniqueMultioutMethods, generateVersion);
		val interfaceName =  francaIntf.joynrName
		val asyncClassName = interfaceName + "Async"

		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".", generateVersion)
		val hasReadAttribute = hasReadAttribute(francaIntf);
		val hasWriteAttribute = hasWriteAttribute(francaIntf);
		'''
«warning()»
package «packagePath»;

import io.joynr.messaging.MessagingQos;
«IF getMethods(francaIntf).size > 0 || hasReadAttribute»
import io.joynr.proxy.Callback;
«IF francaIntf.hasMethodWithErrorEnum»
import io.joynr.proxy.ICallbackWithModeledError;
import io.joynr.proxy.CallbackWithModeledError;
«ENDIF»
import io.joynr.proxy.Future;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
«ENDIF»
«IF uniqueMultioutMethods.size > 0»
import io.joynr.proxy.ICallback;
«ENDIF»
import io.joynr.Async;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;
«IF hasWriteAttribute»
import io.joynr.exceptions.DiscoveryException;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(francaIntf, true, true, true, false, false, false, generateVersion)»
	import «datatype»;
«ENDFOR»

«FOR method: uniqueMultioutMethods»
«val syncReturnedName = methodToSyncReturnedName.get(method)»
	import «packagePath».«interfaceName»Sync.«syncReturnedName»;
«ENDFOR»

@Async
@ProvidedBy(«francaIntf.providerClassName».class)
@UsedBy(«francaIntf.proxyClassName».class)
public interface «asyncClassName» extends «interfaceName»«IF hasFireAndForgetMethods(francaIntf)», «interfaceName»FireAndForget«ENDIF» {

	«FOR attribute: getAttributes(francaIntf)»
		«var attributeName = attribute.joynrName»
		«var attributeType = attribute.typeName.objectDataTypeForPlainType»
		«var getAttribute = "get" + attributeName.toFirstUpper»
		«var setAttribute = "set" + attributeName.toFirstUpper»
		«IF isReadable(attribute)»
			public Future<«attributeType»> «getAttribute»(@JoynrRpcCallback(deserializationType = «attributeType»«IF isArray(attribute)»[]«ENDIF».class) Callback<«attributeType»> callback);
			default public Future<«attributeType»> «getAttribute»(@JoynrRpcCallback(deserializationType = «attributeType»«IF isArray(attribute)»[]«ENDIF».class) Callback<«attributeType»> callback, MessagingQos messagingQos) {
				return «getAttribute»(callback);
			}
		«ENDIF»
		«IF isWritable(attribute)»
			Future<Void> «setAttribute»(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback, «attributeType» «attributeName») throws DiscoveryException;
			default Future<Void> «setAttribute»(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback, «attributeType» «attributeName», MessagingQos messagingQos) throws DiscoveryException {
				return «setAttribute»(callback, «attributeName»);
			}
		«ENDIF»
	«ENDFOR»

	«FOR method: uniqueMultioutMethods»
	«val futureName = methodToFutureName.get(method)»
	«val syncReturnedName = methodToSyncReturnedName.get(method)»
	public static class «futureName» extends Future<«syncReturnedName»> {
		public void resolve(Object... outParameters) {
			if (outParameters.length == 0) {
				onSuccess(null);
			} else {
				onSuccess(new «syncReturnedName»(outParameters));
			}
		}

		public static Class<?>[] getDatatypes() {
			return «syncReturnedName».getDatatypes();
		}
	}
	«ENDFOR»

	«FOR method: uniqueMultioutMethods»
		«val callbackName = methodToCallbackName.get(method)»
		«val outputParametersLength = method.outputParameters.length»
		«IF method.hasErrorEnum»
			«IF method.errors !== null»
				«val errorEnumType = packagePath + "." + interfaceName + "." + methodToErrorEnumName.get(method)»
				public abstract class «callbackName» implements ICallback, ICallbackWithModeledError<«errorEnumType»> {
			«ELSE»
				«val errorEnumType = method.errorEnum.buildPackagePath(".", true, generateVersion) + "." + method.errorEnum.joynrName»
				public abstract class «callbackName» implements ICallback, ICallbackWithModeledError<«errorEnumType»> {
			«ENDIF»
		«ELSE»
		public abstract class «callbackName» implements ICallback {
		«ENDIF»
			public abstract void onSuccess(«method.commaSeperatedTypedOutputParameterList»);

			public void resolve(Object... outParameters) {
				if (outParameters.length < «outputParametersLength») {
					onSuccess(«FOR i : 0 ..< outputParametersLength SEPARATOR ", "»null«ENDFOR»);
				} else {
						«var index = 0»
						onSuccess(
							«FOR outParameter : method.outputParameters»
								(«outParameter.typeName») outParameters[«index++»]«IF index < method.outputParameters.length»,«ENDIF»
							«ENDFOR»
					);
				}
			}
		}
	«ENDFOR»

	«FOR method: getMethods(francaIntf).filter[!fireAndForget]»
		«var methodName = method.joynrName»
		«var params = method.inputParameters.typedParameterList»
		«var callbackParameter = getCallbackParameter(method, methodToCallbackName)»

		/*
		* «methodName»
		*/
		public «methodToFutureName.get(method)» «methodName»(
				«callbackParameter»«IF !method.inputParameters.empty»,«ENDIF»
				«params»
		);
		default public «methodToFutureName.get(method)» «methodName»(
				«callbackParameter»«IF !method.inputParameters.empty»,«ENDIF»
				«params»,
				MessagingQos messagingQos
		) {
			return «methodName»(
				callback«IF !method.inputParameters.empty»,«ENDIF»
				«FOR inParameter : method.inputParameters SEPARATOR ","»
				«inParameter.name»
				«ENDFOR»
			);
		}

	«ENDFOR»
}
		'''
	}

	def getCallbackParameter(FMethod method, HashMap<FMethod, String> methodToCallbackName) {
		var outputParameterType = method.typeNamesForOutputParameter.iterator.next;
		var outputObjectType = getObjectDataTypeForPlainType(outputParameterType);
		var callbackType = methodToCallbackName.get(method);
		if (method.outputParameters.size < 2) {
			if (outputParameterType!="void"){
				if (outputObjectType == ""){
					throw new IllegalArgumentException("error in method: " + method
						+ ". outputObjectType is empty even though outputParameterType is not void")
				}
				else{
					return "@JoynrRpcCallback(deserializationType = " + outputObjectType + ".class) " + callbackType + " callback"
				}
			} else {
				return "@JoynrRpcCallback(deserializationType = Void.class) " + callbackType + " callback"
			}
		} else {
			return "@JoynrRpcCallback " + callbackType + " callback"
		}
	}
}
