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

class InterfaceProviderTemplate implements InterfaceTemplate{
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension InterfaceUtil
	@Inject extension MethodUtil
	@Inject extension AttributeUtil
	@Inject extension NamingUtil
	@Inject extension TemplateBase

	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToDeferredName) {
		init(serviceInterface, methodToDeferredName, new ArrayList<FMethod>());
	}

	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToDeferredName, ArrayList<FMethod> uniqueMethodsToCreateDeferreds) {
		var uniqueMethodSignatureToPromiseName = new HashMap<String, String>();
		var methodNameToCount = overloadedMethodCounts(getMethods(serviceInterface));
		var methodNameToIndex = new HashMap<String, Integer>();

		for (FMethod method : getMethods(serviceInterface)) {
			if (method.outputParameters.isEmpty && !method.hasErrorEnum()) {
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
				val methodSignature = createMethodSignatureFromOutParameters(method);
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
		var methodToErrorEnumName = serviceInterface.methodToErrorEnumName
		var uniqueMethodsToCreateDeferreds = new ArrayList<FMethod>();
		init(serviceInterface, methodToDeferredName, uniqueMethodsToCreateDeferreds);

		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "Provider"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''
«warning()»
package «packagePath»;

«IF getMethods(serviceInterface).size > 0 || hasReadAttribute(serviceInterface)»
	import io.joynr.provider.Promise;
«ENDIF»
«IF hasReadAttribute(serviceInterface)»
	import io.joynr.provider.Deferred;
«ENDIF»
«IF !uniqueMethodsToCreateDeferreds.isEmpty»
	import io.joynr.provider.AbstractDeferred;
«ENDIF»
«IF hasWriteAttribute(serviceInterface) || hasMethodWithArguments(serviceInterface)»
	import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
«ENDIF»
«IF hasWriteAttribute(serviceInterface) || hasMethodWithoutReturnValue(serviceInterface)»
	import io.joynr.provider.DeferredVoid;
«ENDIF»
«IF serviceInterface.hasMethodWithErrorEnum»
	import joynr.exceptions.ApplicationException;
«ENDIF»

import io.joynr.provider.JoynrProvider;

«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	import «datatype»;
«ENDFOR»

public interface «className» extends JoynrProvider {
	public static final String INTERFACE_NAME = "«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName»";
	«FOR attribute : getAttributes(serviceInterface)»
		«var attributeName = attribute.joynrName»
		«var attributeType = attribute.typeName.objectDataTypeForPlainType»

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
		«var params = method.typedParameterListJavaRpc»
		«var comments = method.javadocCommentsParameterListJavaRpc»

		/**
		 * «methodName»
		«IF !comments.equals("")»«comments»«ENDIF»
		 * @return promise for asynchronous handling
		 */
		public Promise<«methodToDeferredName.get(method)»> «methodName»(
				«IF !params.equals("")»«params»«ENDIF»
		);
	«ENDFOR»
	«FOR method : uniqueMethodsToCreateDeferreds»

		public class «methodToDeferredName.get(method)» extends AbstractDeferred {
			«IF method.outputParameters.empty»
				public synchronized boolean resolve() {
					Object[] values = new Object[] {};
					return super.resolve(values);
				}
			««« In the case of single output param that is an array, the varargs resolve gets confused
			««« and assumes the array is multi-out. Cast to object to prevent this from happening.
			«ELSEIF method.outputParameters.length == 1 && (isArray(method.outputParameters.get(0)) || isByteBuffer(method.outputParameters.get(0).type))»
				public synchronized boolean resolve(«method.commaSeperatedTypedOutputParameterList») {
					return super.resolve((Object)«method.commaSeperatedUntypedOutputParameterList»);
				}
			«ELSE»
				public synchronized boolean resolve(«method.commaSeperatedTypedOutputParameterList») {
					return super.resolve(«method.commaSeperatedUntypedOutputParameterList»);
				}
			«ENDIF»
			«IF method.hasErrorEnum()»
				«IF method.errors != null»
					public synchronized boolean reject(«packagePath».«interfaceName».«methodToErrorEnumName.get(method)» error) {
				«ELSE»
					public synchronized boolean reject(«method.errorEnum.buildPackagePath(".", true)».«method.errorEnum.joynrName»«» error) {
				«ENDIF»
					return super.reject(new ApplicationException(error));
				}
			«ENDIF»
		}
	«ENDFOR»
	«FOR broadcast : serviceInterface.broadcasts»
		«val broadcastName = broadcast.joynrName»

		public void fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedTypedOutputParameterList»);
	«ENDFOR»
}
		'''
	}
}
