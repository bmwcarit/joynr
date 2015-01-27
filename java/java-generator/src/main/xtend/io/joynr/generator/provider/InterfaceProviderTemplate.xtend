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
import io.joynr.generator.util.InterfaceTemplate

class InterfaceProviderTemplate implements InterfaceTemplate{
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "Provider"
		val syncClassName = interfaceName + "Sync"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''

		«warning()»
		package «packagePath»;

		import java.util.List;
		import java.util.ArrayList;
		import java.util.Map;

		import io.joynr.provider.JoynrProvider;

		«FOR datatype: getRequiredIncludesFor(serviceInterface)»
			import «datatype»;
		«ENDFOR»
		//TODO: Only include the necessary imports in the xtend template. This needs to be checked depending on the franca model. 
		@SuppressWarnings("unused")

		public interface «className» extends «syncClassName» {

		«FOR attribute: getAttributes(serviceInterface)»
			«val attributeName = attribute.joynrName»
			«val attributeType = getMappedDatatypeOrList(attribute)»
			«IF isReadable(attribute)»
				@Override
				public «attributeType» get«attributeName.toFirstUpper»();
			«ENDIF»

			«IF isNotifiable(attribute)»
				public void «attributeName»Changed(«attributeType» «attributeName»);
			«ENDIF»

			«IF isWritable(attribute)»
				@Override
				public void set«attributeName.toFirstUpper»(«attributeType» «attributeName»);
			«ENDIF»
		«ENDFOR»

		«FOR broadcast: serviceInterface.broadcasts SEPARATOR '\n'»
			«val broadcastName = broadcast.joynrName»
			public void «broadcastName»EventOccurred(«getMappedOutputParametersCommaSeparated(broadcast, false)»);
		«ENDFOR»

		}
		'''
	}
}