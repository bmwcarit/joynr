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
import org.franca.core.franca.FMethod
import io.joynr.generator.util.InterfaceTemplate
import java.util.HashMap
import java.util.ArrayList
import org.franca.core.franca.FArgument

class InterfaceProviderTemplate implements InterfaceTemplate{
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToDeferredName) {
		init(serviceInterface, methodToDeferredName, new ArrayList<FMethod>());
	}

	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToDeferredName, ArrayList<FMethod> uniqueMethodsToCreateDeferreds) {
		var uniqueMethodSignatureToPromiseName = new HashMap<String, String>();
		var methodNameToCount = overloadedMethodCounts(getMethods(serviceInterface));
		var methodNameToIndex = new HashMap<String, Integer>();

		for (FMethod method : getMethods(serviceInterface)) {
			if (method.outputParameters.isEmpty) {
				// void method
				methodToDeferredName.put(method, "DeferredVoid");
			} else if (methodNameToCount.get(method.name) == 1) {
				// method not overloaded, so no index needed
				methodToDeferredName.put(method, method.name.toFirstUpper + "Deferred");
				uniqueMethodsToCreateDeferreds.add(method);
			} else {
				// initialize index if not existent
				if (!methodNameToIndex.containsKey(method.name)) {
					methodNameToIndex.put(method.name, 0);
				}
				val methodSignature = createMethodSignature(method);
				if (!uniqueMethodSignatureToPromiseName.containsKey(methodSignature)) {
					var Integer index = methodNameToIndex.get(method.name);
					index++;
					methodNameToIndex.put(method.name, index);
					uniqueMethodSignatureToPromiseName.put(methodSignature, method.name.toFirstUpper + index);
					uniqueMethodsToCreateDeferreds.add(method);
				}

				methodToDeferredName.put(method, uniqueMethodSignatureToPromiseName.get(methodSignature) + "Deferred");
			}
		}
	}

	override generate(FInterface serviceInterface) {
		var methodToDeferredName = new HashMap<FMethod, String>();
		var uniqueMethodsToCreateDeferreds = new ArrayList<FMethod>();
		init(serviceInterface, methodToDeferredName, uniqueMethodsToCreateDeferreds);

		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "Provider"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''
«warning()»
package «packagePath»;

«IF needsListImport(serviceInterface)»
	import java.util.List;
«ENDIF»
import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
«IF getMethods(serviceInterface).size > 0 || hasReadAttribute(serviceInterface)»
	import io.joynr.provider.Promise;
«ENDIF»
«IF hasReadAttribute(serviceInterface)»
	import io.joynr.provider.Deferred;
«ENDIF»
«IF !getMethods(serviceInterface).isEmpty»
	import io.joynr.dispatcher.rpc.Callback;
	import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
«ENDIF»
// TODO: remove end
«IF !uniqueMethodsToCreateDeferreds.isEmpty»
	import io.joynr.provider.AbstractDeferred;
«ENDIF»
«IF hasWriteAttribute(serviceInterface) || hasMethodWithArguments(serviceInterface)»
	import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
«ENDIF»
«IF hasWriteAttribute(serviceInterface) || hasMethodWithoutReturnValue(serviceInterface)»
	import io.joynr.provider.DeferredVoid;
«ENDIF»

import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.provider.JoynrProvider;

«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	import «datatype»;
«ENDFOR»

public interface «className» extends JoynrInterface, JoynrProvider {
	public static final String INTERFACE_NAME = "«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName.toLowerCase»";
	«FOR attribute : getAttributes(serviceInterface)»
		«var attributeName = attribute.joynrName»
		«var attributeType = getObjectDataTypeForPlainType(getMappedDatatypeOrList(attribute))»

		«IF isReadable(attribute)»
			Promise<Deferred<«attributeType»>> get«attributeName.toFirstUpper»();
		«ENDIF»
		«IF isWritable(attribute)»
			Promise<DeferredVoid> set«attributeName.toFirstUpper»(«attributeType» «attributeName»);
		«ENDIF»
		«IF isNotifiable(attribute)»
			public void «attributeName»Changed(«attributeType» «attributeName»);
		«ENDIF»
	«ENDFOR»
	«FOR method : getMethods(serviceInterface)»
		«var methodName = method.joynrName»
		«var params = getTypedParameterListJavaRpc(method)»

		/**
		 * «methodName»
		 */
		public Promise<«methodToDeferredName.get(method)»> «methodName»(
				«IF !params.equals("")»«params»«ENDIF»
		);
	«ENDFOR»
	«FOR method : uniqueMethodsToCreateDeferreds»

		public class «methodToDeferredName.get(method)» extends AbstractDeferred {
		«IF method.outputParameters.empty»
			public synchronized boolean resolve() {
				values = new Object[] {};
				return super.resolve();
			}
		«ELSE»
			«var outParameterName = method.outputParameters.iterator.next.name»
			«var outParameterType = getObjectDataTypeForPlainType(method.outputParameters.iterator.next.mappedDatatypeOrList.objectDataTypeForPlainType)»
			public synchronized boolean resolve(«outParameterType» «outParameterName») {
				values = new Object[] { «outParameterName» };
				return super.resolve();
			}
		«ENDIF»
		}
	«ENDFOR»
	«FOR broadcast : serviceInterface.broadcasts»
		«val broadcastName = broadcast.joynrName»

		public void fire«broadcastName.toFirstUpper»(«getMappedOutputParametersCommaSeparated(broadcast, false)»);
	«ENDFOR»
}
		'''
	}

	/**
	 * @return a message signature that is unique in terms of method name, out
	 *      parameter names and out parameter types.
	 */
	def createMethodSignature(FMethod method) {
		val nameStringBuilder = new StringBuilder(method.name);
		for (FArgument outParam : method.outputParameters) {
			nameStringBuilder.append(outParam.name.toFirstUpper);
			val typeName = new StringBuilder(outParam.mappedDatatypeOrList.objectDataTypeForPlainType);
			if (typeName.toString().contains("List")) {
				typeName.deleteCharAt(4);
				typeName.deleteCharAt(typeName.length-1);
			}
			nameStringBuilder.append(typeName.toString());
		}
		return nameStringBuilder.toString;
	}

	/**
	 * @return a mapping from method names to the number of their overloads.
	 */
	def overloadedMethodCounts(Iterable<FMethod> methods) {
		var methodNameToCount = new HashMap<String, Integer>();

		for (FMethod method : methods) {
			var Integer count = 1;
			if (methodNameToCount.containsKey(method.name)) {
				count = methodNameToCount.get(method.name);
				count++;
			}
			methodNameToCount.put(method.name, count);
		}
		return methodNameToCount;
	}
}
