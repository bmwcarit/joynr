package io.joynr.generator.js.util

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
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.JoynrGeneratorExtensions
import org.franca.core.franca.FAttribute
import java.util.stream.Collectors
import java.util.List

class JoynrJSGeneratorExtensions extends JoynrGeneratorExtensions {
	@Inject extension AttributeUtil

	def getAttributeCaps(FAttribute attribute)
	'''«IF isNotifiable(attribute)»NOTIFY«ENDIF»«IF isReadable(attribute)»READ«ENDIF»«IF isWritable(attribute)»WRITE«ENDIF»'''
	def getProviderAttributeName(FAttribute attribute)
	'''ProviderRead«IF isWritable(attribute)»Write«ENDIF»«IF isNotifiable(attribute)»Notify«ENDIF»Attribute'''
	def List<String> getProviderAttributeNames(List<FAttribute> attributes){
		attributes.map[it.providerAttributeName.toString()].stream().distinct().collect(Collectors.toList())
	}
	def getProxyAttributeName(FAttribute attribute)
	'''ProxyRead«IF isWritable(attribute)»Write«ENDIF»«IF isNotifiable(attribute)»Notify«ENDIF»Attribute'''
	def List<String> getProxyAttributeNames(List<FAttribute> attributes){
		attributes.map[it.proxyAttributeName.toString()].stream().distinct().collect(Collectors.toList())
	}
	def getProviderAttributeImplName(FAttribute attribute)
	'''ProviderRead«IF isWritable(attribute)»Write«ENDIF»«IF isNotifiable(attribute)»Notify«ENDIF»AttributeImpl'''

	def List<String> getJoynrProviderImports(List<FAttribute> attributes){
		var list = attributes.map[it.providerAttributeImplName.toString()].stream().distinct().collect(Collectors.toList());
		list.add("JoynrProvider");
		return list;
	}
}
