package io.joynr.generator.communicationmodel
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
import io.joynr.generator.util.EnumTemplate
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FEnumerationType

class EnumTypeTemplate implements EnumTemplate{

	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension TemplateBase

	override generate(FEnumerationType enumType)
'''
«val packagePath = enumType.buildPackagePath(".", true)»
«warning()»

package «packagePath»;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

«generateEnumCode(enumType)»
'''

	def generateEnumCode(FEnumerationType enumType)
'''
«val typeName = enumType.joynrName»
/**
«appendJavadocSummaryAndWriteSeeAndDescription(enumType, " *")»
 */
public enum «typeName» {
	«FOR enumValue : getEnumElementsAndBaseEnumElements(enumType) SEPARATOR ","»
	/**
	 * «appendJavadocComment(enumValue, "* ")»
	 */
	«enumValue.joynrName»
	«ENDFOR»;

	static final Map<Integer, «typeName»> ordinalToEnumValues = new HashMap<Integer, «typeName»>();

	static{
		«var i = -1»
		«FOR enumValue : getEnumElementsAndBaseEnumElements(enumType)»
		ordinalToEnumValues.put(Integer.valueOf(«IF enumValue.value==null|| enumValue.value.equals("")»«i=i+1»«ELSE»«enumValue.value»«ENDIF»), «enumValue.joynrName»);
		«ENDFOR»
	}

	/**
	 * Get the matching enum for an ordinal number
	 * @param ordinal The ordinal number
	 * @return The matching enum for the given ordinal number
	 */
	public static «typeName» getEnumValue(Integer ordinal) {
		return ordinalToEnumValues.get(ordinal);
	}

	/**
	 * Get the matching ordinal number for this enum
	 * @return The ordinal number representing this enum
	 */
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