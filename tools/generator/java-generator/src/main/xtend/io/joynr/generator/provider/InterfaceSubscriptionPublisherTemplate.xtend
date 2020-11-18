package io.joynr.generator.provider
/*
 * !!!
 *
 * Copyright (C) 2017 BMW Car IT GmbH
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
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase

class InterfaceSubscriptionPublisherTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil
	@Inject extension AttributeUtil
	@Inject extension TemplateBase

	override generate(boolean generateVersion) {
		val interfaceName =  francaIntf.joynrName
		val className = interfaceName + "SubscriptionPublisher"
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".", generateVersion)

		'''
«warning()»
package «packagePath»;

«IF hasNonSelectiveBroadcast»
	import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
«ENDIF»
import io.joynr.provider.SubscriptionPublisher;

«FOR datatype : getRequiredIncludesFor(francaIntf, false, false, false, true, true, false, generateVersion)»
	import «datatype»;
«ENDFOR»

public interface «className» extends SubscriptionPublisher {

	«FOR attribute : getAttributes(francaIntf)»
		«val attributeName = attribute.joynrName»
		«val attributeType = attribute.typeName»
		«IF isNotifiable(attribute)»
			public void «attributeName»Changed(«attributeType» «attributeName»);
		«ENDIF»
	«ENDFOR»

	«FOR broadcast : francaIntf.broadcasts»
		«var broadcastName = broadcast.joynrName»
		«IF broadcast.selective»
		public void fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedTypedOutputParameterList»);
		«ELSE»
		@JoynrMulticast(name = "«broadcastName»")
		public void fire«broadcastName.toFirstUpper»(«broadcast.commaSeperatedTypedOutputParameterList»«IF broadcast.outputParameters.length > 0», «ENDIF»String... partitions);
		«ENDIF»
	«ENDFOR»
}
		'''
	}
}
