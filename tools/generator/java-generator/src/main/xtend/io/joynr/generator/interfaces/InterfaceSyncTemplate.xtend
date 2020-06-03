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

class InterfaceSyncTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension MethodUtil
	@Inject extension AttributeUtil
	@Inject extension InterfaceUtil
	@Inject extension NamingUtil
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

	override generate() {
		var methodToReturnTypeName = new HashMap<FMethod, String>();
		var uniqueMultioutMethods = new ArrayList<FMethod>();
		init(francaIntf, methodToReturnTypeName, uniqueMultioutMethods);
		val interfaceName =  francaIntf.joynrName
		val syncClassName = interfaceName + "Sync"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".")
		'''
«warning()»

package «packagePath»;

import io.joynr.messaging.MessagingQos;
import io.joynr.Sync;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;
«IF hasMethodWithErrorEnum(francaIntf)»
	import joynr.exceptions.ApplicationException;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(francaIntf, true, true, true, false, false, false)»
	import «datatype»;
«ENDFOR»

@Sync
@ProvidedBy(«francaIntf.providerClassName».class)
@UsedBy(«francaIntf.proxyClassName».class)
public interface «syncClassName» extends «interfaceName»«IF hasFireAndForgetMethods(francaIntf)», «interfaceName»FireAndForget«ENDIF» {

«FOR attribute: getAttributes(francaIntf) SEPARATOR "\n"»
	«var attributeName = attribute.joynrName»
	«var attributeType = attribute.typeName.objectDataTypeForPlainType»
	«var getAttribute = "get" + attributeName.toFirstUpper»
	«var setAttribute = "set" + attributeName.toFirstUpper»
		«IF isReadable(attribute)»
			public «attributeType» «getAttribute»();
			default public «attributeType» «getAttribute»(MessagingQos messagingQos) {
				return «getAttribute»();
			}
		«ENDIF»
		«IF isWritable(attribute)»
			void «setAttribute»(«attributeType» «attributeName»);
			default void «setAttribute»(«attributeType» «attributeName», MessagingQos messagingQos) {
			}
		«ENDIF»
«ENDFOR»

«FOR method: uniqueMultioutMethods»
	«val containerName = methodToReturnTypeName.get(method)»
		public class «containerName» implements io.joynr.dispatcher.rpc.MultiReturnValuesContainer {
			«FOR outParameter : method.outputParameters»
				public final «outParameter.typeName» «outParameter.name»;
			«ENDFOR»
			public «containerName»(Object... outParameters) {
				«var index = 0»
				«FOR outParameter : method.outputParameters»
					this.«outParameter.name» = («outParameter.typeName») outParameters[«index++»];
				«ENDFOR»
			}

			public static Class<?>[] getDatatypes() {
				return new Class<?>[] {«FOR outParameter : method.outputParameters SEPARATOR ", "»«outParameter.typeName».class«ENDFOR»};
			}

			public Object[] getValues() {
			    return new Object[] {
			        «FOR outParameter : method.outputParameters SEPARATOR ","»
			            «outParameter.name»
			        «ENDFOR»
			    };
			}
		}
«ENDFOR»

«FOR method: getMethods(francaIntf).filter[!fireAndForget] SEPARATOR "\n"»
	«var methodName = method.joynrName»
		/*
		* «methodName»
		*/
		public «methodToReturnTypeName.get(method)» «methodName»(
				«method.inputParameters.typedParameterList»
		)«IF method.hasErrorEnum» throws ApplicationException«ENDIF»;
		default public «methodToReturnTypeName.get(method)» «methodName»(
				«method.inputParameters.typedParameterList»«IF !method.inputParameters.empty»,«ENDIF»
				MessagingQos messagingQos
		)«IF method.hasErrorEnum» throws ApplicationException«ENDIF» {
			«IF methodToReturnTypeName.get(method).equals("void")»
			return;
			«ELSE»
			return «methodName»(
				«FOR inParameter : method.inputParameters SEPARATOR ","»
					«inParameter.name»
				«ENDFOR»
			);
			«ENDIF»
		}
«ENDFOR»
}
		'''
	}

}
