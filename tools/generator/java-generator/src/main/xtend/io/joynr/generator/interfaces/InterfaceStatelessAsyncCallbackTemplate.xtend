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
 *	  http://www.apache.org/licenses/LICENSE-2.0
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
import java.util.HashSet

class InterfaceStatelessAsyncCallbackTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension MethodUtil
	@Inject extension InterfaceUtil
	@Inject extension NamingUtil
	@Inject extension TemplateBase
	@Inject extension AttributeUtil

	override generate() {
		val interfaceName =  francaIntf.joynrName
		val statelessAsyncClassName = interfaceName + "StatelessAsyncCallback"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".")
		var methodToErrorEnumName = francaIntf.methodToErrorEnumName
		'''
«warning()»

package «packagePath»;

import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.UsedBy;

«FOR datatype: getRequiredCallbackIncludesFor(francaIntf)»
	import «datatype»;
«ENDFOR»

@UsedBy(«francaIntf.proxyClassName».class)
public interface «statelessAsyncClassName» extends StatelessAsyncCallback {

«FOR attribute: getAttributes(francaIntf) SEPARATOR "\n"»
	«var attributeName = attribute.joynrName»
	«var attributeType = attribute.typeName.objectDataTypeForPlainType»
	«var getAttribute = "get" + attributeName.toFirstUpper»
	«var setAttribute = "set" + attributeName.toFirstUpper»
		«IF isReadable(attribute)»
		/*
		* «attributeName» getter
		*/
		@StatelessCallbackCorrelation("«getAttribute.hashCode»")
		default void «getAttribute»Success(«attributeType» «attributeName», String messageId)
		{ throw new UnsupportedOperationException("«getAttribute»Success not implemented for callback instance"); }
		«ENDIF»
		«IF isWritable(attribute)»
		/*
		* «attributeName» setter
		*/
		@StatelessCallbackCorrelation("«setAttribute.hashCode»")
		default void «setAttribute»Success(String messageId)
		{ throw new UnsupportedOperationException("«setAttribute»Success not implemented for callback instance"); }
		«ENDIF»
«ENDFOR»

«var successMethodsGenerated = new HashSet<String>()»
«var failedMethodsGenerated = new HashSet<String>()»
«FOR method: getMethods(francaIntf).filter[!fireAndForget] SEPARATOR "\n"»
	«var methodSignature = method.createMethodSignatureFromOutParameters»
	«var methodName = method.joynrName»
		«IF successMethodsGenerated.add(methodSignature)»
		/*
		* «methodName»
		*/
		@StatelessCallbackCorrelation("«methodSignature.hashCode»")
		default void «methodName»Success(
				«IF method.outputParameters.size()>0»
				«method.outputParameters.typedParameterList»,
				«ENDIF»
				String messageId
		) { throw new UnsupportedOperationException("«methodName»Success not implemented for callback instance"); }
		«ENDIF»
		«IF method.hasErrorEnum && failedMethodsGenerated.add(methodSignature)»
		@StatelessCallbackCorrelation("«methodSignature.hashCode»")
		default void «methodName»Failed(
			«IF method.errors !== null»
				«val errorEnumType = packagePath + "." + interfaceName + "." + methodToErrorEnumName.get(method)»
				«errorEnumType» error,
			«ELSE»
				«val errorEnumType = method.errorEnum.buildPackagePath(".", true) + "." + method.errorEnum.joynrName»
				«errorEnumType» error,
			«ENDIF»
				String messageId
		) { throw new UnsupportedOperationException("«methodName»Failed not implemented for callback instance"); }
		«ENDIF»
«ENDFOR»
}
		'''
	}

}
