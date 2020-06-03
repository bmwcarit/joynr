package io.joynr.generator.interfaces
/*
 * !!!
 *
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

class InterfaceStatelessAsyncTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension MethodUtil
	@Inject extension InterfaceUtil
	@Inject extension NamingUtil
	@Inject extension TemplateBase
	@Inject extension AttributeUtil

	override generate() {
		val interfaceName =  francaIntf.joynrName
		val statelessAsyncClassName = interfaceName + "StatelessAsync"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".")
		'''
«warning()»

package «packagePath»;

import io.joynr.StatelessAsync;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.proxy.MessageIdCallback;
import io.joynr.UsedBy;

«FOR datatype: getRequiredStatelessAsyncIncludesFor(francaIntf)»
	import «datatype»;
«ENDFOR»

@StatelessAsync
@UsedBy(«francaIntf.proxyClassName».class)
public interface «statelessAsyncClassName» extends «interfaceName»«IF hasFireAndForgetMethods(francaIntf)», «interfaceName»FireAndForget«ENDIF» {

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
		void «getAttribute»(MessageIdCallback messageIdCallback);
		«ENDIF»
		«IF isWritable(attribute)»
		/*
		* «attributeName» setter
		*/
		@StatelessCallbackCorrelation("«setAttribute.hashCode»")
		void «setAttribute»(«attributeType» «attributeName», MessageIdCallback messageIdCallback);
		«ENDIF»
«ENDFOR»

«FOR method: getMethods(francaIntf).filter[!fireAndForget] SEPARATOR "\n"»
	«var methodName = method.joynrName»
		/*
		* «methodName»
		*/
		@StatelessCallbackCorrelation("«method.createMethodSignatureFromOutParameters.hashCode»")
		void «methodName»(
				«IF method.inputParameters.size()>0»
				«method.inputParameters.typedParameterList»,
				«ENDIF»
				MessageIdCallback messageIdCallback
		);
«ENDFOR»
}
		'''
	}

}
