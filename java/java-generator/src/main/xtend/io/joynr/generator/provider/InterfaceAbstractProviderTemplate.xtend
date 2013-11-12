package io.joynr.generator.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

class InterfaceAbstractProviderTemplate {
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName + "AbstractProvider"
		val providerInterfaceName = interfaceName + "Provider"
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		

		'''
		«warning()»
		package «packagePath»;

		import java.util.List;
		import java.util.ArrayList;
		import java.util.Map;

		import io.joynr.provider.AbstractJoynrProvider;
		import «joynTypePackagePrefix».types.ProviderQos;
		
		
		«FOR datatype: getRequiredIncludesFor(serviceInterface)»
			import «datatype»;
		«ENDFOR»
			
			
		//TODO: Only include the necessary imports in the xtend template. This needs to be checked depending on the fibex. 
		@SuppressWarnings("unused")

		public abstract class «className» extends AbstractJoynrProvider implements «providerInterfaceName» {
			    protected ProviderQos providerQos = new ProviderQos();
			    

		«IF getAttributes(serviceInterface).size() > 0»
			//attributes
		«ENDIF»
		«FOR attribute: getAttributes(serviceInterface)»
			«val attributeName = attribute.joynrName»
			«val attributeType = getMappedDatatypeOrList(attribute)»
				protected «attributeType» «attributeName»;	
		«ENDFOR»
		
		«IF getAttributes(serviceInterface).size() > 0»
		 	//setter & abstract getter
		«ENDIF»
		«FOR attribute: getAttributes(serviceInterface)»
			«val attributeName = attribute.joynrName»
			«val attributeType = getMappedDatatypeOrList(attribute)»
	

			«IF isReadable(attribute)»
				@Override
				public abstract «attributeType» get«attributeName.toFirstUpper»();
			«ENDIF»
		
			«IF isNotifiable(attribute)»
				@Override
				public final void «attributeName.toFirstLower»Changed(«attributeType» «attributeName») {
					this.«attributeName» = «attributeName»;
					onAttributeValueChanged("«attributeName»", this.«attributeName»);
				}
			«ENDIF»

			«IF isWritable(attribute)»
				@Override
				public abstract void set«attributeName.toFirstUpper»(«attributeType» «attributeName»);
			«ENDIF»
		«ENDFOR»
		
			@Override
			public ProviderQos getProviderQos() {
			return providerQos;
			}
		
			}
		'''	
	}		
}