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

class InterfaceProviderImplTemplate implements InterfaceTemplate{
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject	extension TemplateBase
	
	override generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = "Default" + interfaceName + "Provider"
		val abstractProviderName = interfaceName + "AbstractProvider"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")

		'''
		«warning()»
		package «packagePath»;
		import java.util.List;
		import java.util.ArrayList;
		import java.util.UUID;
		import com.google.inject.Singleton;
		import org.slf4j.Logger;
		import org.slf4j.LoggerFactory;

		import «joynTypePackagePrefix».types.ProviderQos;

		«FOR datatype: getRequiredIncludesFor(serviceInterface)»
			import «datatype»;
		«ENDFOR»
		//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
		//Therefore some imports migth be unused in this version of the interface.
		//To prevent warnings @SuppressWarnings("unused") is being used. 
		//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
		import java.util.TreeSet;
		@SuppressWarnings("unused")

		@Singleton
		public class «className» extends «abstractProviderName» {
			private static final Logger logger = LoggerFactory.getLogger(«className».class);

			public «className»() {				
				// default uses a priority that is the current time, causing arbitration to the last started instance
				//providerQos.put(ArbitrationConstants.PRIORITY_PARAMETER, "" + System.currentTimeMillis());
				providerQos.setPriority(System.currentTimeMillis());
			}	

			«FOR attribute: getAttributes(serviceInterface)»
				«val attributeName = attribute.joynrName»
				«val attributeType = getMappedDatatypeOrList(attribute)»

				«IF isReadable(attribute)»
					@Override
					public «attributeType» get«attributeName.toFirstUpper»() {
						return «attributeName»;
					}
				«ENDIF»
				«IF isWritable(attribute)»
					@Override
					public void set«attributeName.toFirstUpper»(«attributeType» «attributeName») {
						«IF isNotifiable(attribute)»
							super.«attributeName»Changed(«attributeName»);
						«ENDIF»
						this.«attributeName» = «attributeName»;
					}
				«ENDIF»
			«ENDFOR»

		«FOR method: getMethods(serviceInterface)»
		«val methodName = method.joynrName»
		«val outputParameters = getOutputParameters(method)»
			«val outputParameterType = mapOutputParameters(outputParameters).iterator.next»
			«val outputParameter = if (!outputParameters.isEmpty) outputParameters.iterator.next else null»
			@Override
			public «outputParameterType» «methodName»(«getCommaSeperatedTypedParameterList(method)») {
				logger.warn("**********************************************");
				logger.warn("* «interfaceName».«methodName» called");
				logger.warn("**********************************************");	

			«IF outputParameterType=="void"»
			«ELSEIF outputParameterType=="String"»
				return "Hello World";
			«ELSEIF outputParameterType=="Boolean"»
				return false;
			«ELSEIF outputParameterType=="Integer"»
				return 42;
			«ELSEIF outputParameterType=="Double"»
			    return 3.1415;
			«ELSEIF outputParameterType=="Long"»
			    return (long) 42;
			«ELSEIF outputParameterType=="Byte"»
			    return (byte) 42;
			«ELSEIF outputParameterType.startsWith("List<")»
				return new Array«outputParameterType»();
			«ELSEIF isEnum(outputParameter.type)»
				return «outputParameterType».«getEnumElements(getEnumType(outputParameter.type)).iterator.next.joynrName»;
			«ELSE»
				return new «outputParameterType»();	
			«ENDIF»

			}
		«ENDFOR»

			@Override
			public ProviderQos getProviderQos() {
			    return providerQos;
			}	
		}
		'''	
	}
}