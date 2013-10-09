package io.joynr.generator.proxy
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

class InterfaceProxyTemplate {
	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	def generate(FInterface fInterface) {
		val interfaceName =  fInterface.name.toFirstUpper
		val className = interfaceName + "Proxy"
		val asyncClassName = interfaceName + "Async"
		val syncClassName = interfaceName + "Sync"
		val subscriptionClassName = interfaceName + "SubscriptionInterface"
		val packagePath = getPackagePathWithJoynrPrefix(fInterface, ".")
		'''

		«warning()»   
		package «packagePath»;  	
			
		public interface «className» extends «asyncClassName», «syncClassName», «subscriptionClassName» {
		    public static String INTERFACE_NAME = "«getPackagePathWithoutJoynrPrefix(fInterface, "/")»/«interfaceName.toLowerCase»";
		}
		'''	
	}	
	
			
}