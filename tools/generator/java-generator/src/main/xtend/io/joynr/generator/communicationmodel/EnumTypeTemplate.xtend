package io.joynr.generator.communicationmodel
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
import io.joynr.generator.templates.EnumTemplate
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.util.JavaTypeUtil
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.TemplateBase
import org.franca.core.franca.FEnumerationType
import com.google.inject.assistedinject.Assisted

class EnumTypeTemplate extends EnumTemplate {

	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension NamingUtil
	@Inject extension TemplateBase

	@Inject
	new(@Assisted FEnumerationType type) {
		super(type)
	}

	override generate(boolean generateVersion)
'''
«val packagePath = type.buildPackagePath(".", true, generateVersion)»
«warning()»

package «packagePath»;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

«generateEnumCode()»
'''

	def generateEnumCode()
'''
«val typeName = type.joynrName»
/**
«appendJavadocSummaryAndWriteSeeAndDescription(type, " *")»
 */
public enum «typeName» {
	«FOR enumValue : getEnumElementsAndBaseEnumElements(type) SEPARATOR ","»
	/**
	 * «appendJavadocComment(enumValue, "* ")»
	 */
	«enumValue.joynrName»
	«ENDFOR»;

	public static final int MAJOR_VERSION = «majorVersion»;
	public static final int MINOR_VERSION = «minorVersion»;
	static final Map<Integer, «typeName»> ordinalToEnumValues = new HashMap<>();

	static {
		«var ordinal = -1»
		«FOR enumValue : getEnumElementsAndBaseEnumElements(type)»
			«{
				ordinal = if (enumValue.value.enumeratorValue === null)
							ordinal+1
						else
							Integer::valueOf(enumValue.value.enumeratorValue);
				""
			}»
			ordinalToEnumValues.put(«ordinal», «enumValue.joynrName»);
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
		for (Entry<Integer, «typeName»> entry : ordinalToEnumValues.entrySet()) {
			if (this == entry.getValue()) {
				ordinal = entry.getKey();
				break;
			}
		}
		return ordinal;
	}
}
'''
}
