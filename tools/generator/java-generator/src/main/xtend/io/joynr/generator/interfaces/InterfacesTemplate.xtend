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
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTemplateFactory
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase

class InterfacesTemplate extends InterfaceTemplate {
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension NamingUtil
	@Inject extension InterfaceUtil
	@Inject extension TemplateBase
	@Inject JavaTemplateFactory templateFactory

	override generate() {
		val interfaceName =  francaIntf.joynrName
		val className = interfaceName
		val packagePath = getPackagePathWithJoynrPrefix(francaIntf, ".")
		val hasMethodWithImplicitErrorEnum = hasMethodWithImplicitErrorEnum(francaIntf)
		val methodToErrorEnumName = francaIntf.methodToErrorEnumName()
		'''

		«warning()»
package «packagePath»;

import java.util.List;
«IF hasMethodWithImplicitErrorEnum»
	import java.util.HashMap;
	import java.util.Map;
	import java.util.Map.Entry;
«ENDIF»

«FOR datatype: getRequiredIncludesFor(francaIntf)»
	import «datatype»;
«ENDFOR»

//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used.
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@SuppressWarnings("unused")
public interface «className» {
	public static String INTERFACE_NAME = "«getPackagePathWithoutJoynrPrefix(francaIntf, "/")»/«interfaceName»";

	«FOR method: getMethods(francaIntf)»
		«var enumType = method.errors»
		«IF enumType != null»
			«var enumTypeTemplate = templateFactory.createEnumTypeTemplate(enumType)»
			«enumType.name = methodToErrorEnumName.get(method)»
			«enumTypeTemplate.generateEnumCode()»

		«ENDIF»
	«ENDFOR»
}

'''
	}
}
