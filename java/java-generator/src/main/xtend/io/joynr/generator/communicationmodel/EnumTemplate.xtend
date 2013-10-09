package io.joynr.generator.communicationmodel
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
import org.franca.core.franca.FEnumerationType
import io.joynr.generator.util.JoynrJavaGeneratorExtensions

class EnumTemplate {

	@Inject	extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase
	
	def generate(FEnumerationType enumType) {
		val typeName = enumType.name.toFirstUpper
		val packagePath = getPackagePathWithJoynrPrefix(enumType, ".")
		'''
		«warning()»
		
package «packagePath»;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

public enum «typeName» {
	«FOR enumValue : getEnumElements(enumType) SEPARATOR ","»
	«enumValue.name»
	«ENDFOR»;
	
	public static final Map<Integer, «typeName»> ordinalToEnumValues = new HashMap<Integer, «typeName»>();
	
	static{
		«var i = -1»
		«FOR enumValue : getEnumElements(enumType)»
		ordinalToEnumValues.put(Integer.valueOf(«IF enumValue.value==null|| enumValue.value.equals("")»«i=i+1»«ELSE»«enumValue.value»«ENDIF»), «enumValue.name»);
		«ENDFOR»
	}
	
	public static «typeName» getEnumValue(Integer ordinal) {
		return ordinalToEnumValues.get(ordinal);
	}
	
	public Integer getOrdinal() {
		// TODO should we use a bidirectional map from a third-party library?
		Integer ordinal = null;
		for(Entry<Integer, «typeName»> entry : ordinalToEnumValues.entrySet()) {
			if(this == entry.getValue()) {
				ordinal = entry.getKey();
				break;
			}
		}
		return ordinal;
	}
}


'''

	}
}