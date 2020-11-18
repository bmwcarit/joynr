package io.joynr.generator.js.communicationmodel

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
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.templates.util.NamingUtil
import java.util.Date
import org.franca.core.franca.FEnumerationType
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.EnumTemplate
import com.google.inject.assistedinject.Assisted

class EnumTypeGenerator extends EnumTemplate {

	@Inject extension JSTypeUtil
	@Inject extension NamingUtil
	@Inject extension JoynrJSGeneratorExtensions

	@Inject
	new(@Assisted FEnumerationType type) {
		super(type)
	}

	override generate(boolean generateVersion) '''
	«val generationDate = (new Date()).toString»
	«val enumElements = getEnumElementsAndBaseEnumElements(type)»
	«val enumNames = type.joynrName + "Names"»

	import JoynrEnum = require("joynr/joynr/types/JoynrEnum");

	namespace «type.joynrName» {
		export type «enumNames» =«FOR enumElement : enumElements SEPARATOR '|'» "«enumElement.joynrName»"«ENDFOR»
	}

	/**
	 * This is the generated enum type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
	 * Generation date: «generationDate»
	 * 	«appendJSDocSummaryAndWriteSeeAndDescription(type, "* ")»
	 */
	class «type.joynrName» extends JoynrEnum<«type.joynrName».«enumNames»> {
		public static _typeName: string = "«type.getJoynrTypeName(generateVersion)»";
		public _typeName: string = "«type.getJoynrTypeName(generateVersion)»";

		public constructor({name, value}: {name: «type.joynrName».«enumNames», value: «type.joynrName».«enumNames» | number}){
			super(name, value);
		}

		/**
		 * The MAJOR_VERSION of the enum type «type.joynrName» is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MAJOR_VERSION = «majorVersion»;

		/**
		 * The MINOR_VERSION of the enum type «type.joynrName» is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MINOR_VERSION = «minorVersion»;

		«FOR enumValue: enumElements»
		«IF enumValue.comment !== null»
			/**
			 «appendJSDocSummaryAndWriteSeeAndDescription(enumValue, "* ")»
			 */
		«ENDIF»
		public static «enumValue.joynrName» = new «type.joynrName»({
			name: "«enumValue.joynrName»",
			value: «IF enumValue.value===null»"«enumValue.joynrName»"«ELSE»«enumValue.value.enumeratorValue»«ENDIF»
		});
		«ENDFOR»
	}
	export = «type.joynrName»;
	'''
}
