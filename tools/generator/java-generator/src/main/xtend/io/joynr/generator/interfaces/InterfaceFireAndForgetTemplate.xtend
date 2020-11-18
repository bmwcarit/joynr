package io.joynr.generator.interfaces

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
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
 * #L%
 */

import com.google.inject.Inject
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase

class InterfaceFireAndForgetTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension MethodUtil
	@Inject extension InterfaceUtil
	@Inject extension NamingUtil
	@Inject extension TemplateBase

	override generate(boolean generateVersion) {
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".", generateVersion)
		'''
«warning()»

package «packagePath»;

import io.joynr.messaging.MessagingQos;

«FOR datatype: getRequiredIncludesFor(francaIntf, false, false, false, false, false, true, generateVersion)»
	import «datatype»;
«ENDFOR»

@io.joynr.dispatcher.rpc.annotation.FireAndForget
public interface «francaIntf.joynrName + "FireAndForget"» {

«FOR method: getMethods(francaIntf).filter[fireAndForget] SEPARATOR "\n"»
	«var methodName = method.joynrName»
	/*
	 * «methodName»
	 */
	void «methodName»(
			«method.inputParameters.typedParameterList»
	);
	default void «methodName»(
			«method.inputParameters.typedParameterList»«IF !method.inputParameters.empty»,«ENDIF»
			MessagingQos messagingQos
	) {
	};
«ENDFOR»
}
'''
	}

}
