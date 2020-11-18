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

	override generate(boolean generateVersion) {
		val interfaceName =  francaIntf.joynrName
		val statelessAsyncClassName = interfaceName + "StatelessAsyncCallback"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".", generateVersion)
		var methodToErrorEnumName = francaIntf.methodToErrorEnumName
		'''
«warning()»

package «packagePath»;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.ReplyContext;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.UsedBy;

«FOR datatype: getRequiredStatelessAsyncCallbackIncludesFor(francaIntf, generateVersion)»
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
		default void «getAttribute»Success(«attributeType» «attributeName», ReplyContext replyContext)
		{ throw new UnsupportedOperationException("«getAttribute»Success not implemented for callback instance"); }
		@StatelessCallbackCorrelation("«getAttribute.hashCode»")
		default void «getAttribute»Failed(
				JoynrRuntimeException runtimeException,
				ReplyContext replyContext
		) { throw new UnsupportedOperationException("«getAttribute»Failed not implemented for callback instance"); }
		«ENDIF»
		«IF isWritable(attribute)»
		/*
		* «attributeName» setter
		*/
		@StatelessCallbackCorrelation("«setAttribute.hashCode»")
		default void «setAttribute»Success(ReplyContext replyContext)
		{ throw new UnsupportedOperationException("«setAttribute»Success not implemented for callback instance"); }
		@StatelessCallbackCorrelation("«setAttribute.hashCode»")
		default void «setAttribute»Failed(
				JoynrRuntimeException runtimeException,
				ReplyContext replyContext
		) { throw new UnsupportedOperationException("«setAttribute»Failed not implemented for callback instance"); }
		«ENDIF»
«ENDFOR»

«var successMethodsGenerated = new HashSet<String>()»
«var failedWithExceptionMethodsGenerated = new HashSet<String>()»
«var failedWithErrorMethodsGenerated = new HashSet<String>()»
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
				ReplyContext replyContext
		) { throw new UnsupportedOperationException("«methodName»Success not implemented for callback instance"); }
		«ENDIF»
		«IF method.hasErrorEnum && failedWithErrorMethodsGenerated.add(method.createMethodSignatureFromErrors)»
		default void «methodName»Failed(
			«IF method.errors !== null»
				«val errorEnumType = packagePath + "." + interfaceName + "." + methodToErrorEnumName.get(method)»
					«errorEnumType» error,
			«ELSE»
				«val errorEnumType = method.errorEnum.buildPackagePath(".", true, generateVersion) + "." + method.errorEnum.joynrName»
					«errorEnumType» error,
			«ENDIF»
				ReplyContext replyContext
		) { throw new UnsupportedOperationException("«methodName»Failed with error not implemented for callback instance"); }
		«ENDIF»
		«IF failedWithExceptionMethodsGenerated.add(methodName)»
		default void «methodName»Failed(
				JoynrRuntimeException runtimeException,
				ReplyContext replyContext
		) { throw new UnsupportedOperationException("«methodName»Failed with exception not implemented for callback instance"); }
		«ENDIF»
«ENDFOR»
}
		'''
	}

}
