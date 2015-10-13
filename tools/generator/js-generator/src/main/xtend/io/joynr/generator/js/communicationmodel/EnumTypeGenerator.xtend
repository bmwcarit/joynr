package io.joynr.generator.js.communicationmodel

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
import io.joynr.generator.js.util.GeneratorParameter
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import org.franca.core.franca.FEnumerationType
import io.joynr.generator.js.util.JSTypeUtil
import java.util.Date

class EnumTypeGenerator {

	@Inject
	extension JoynrJSGeneratorExtensions

	@Inject
	extension JSTypeUtil

	@Inject
	extension GeneratorParameter

	def generateEnumType(FEnumerationType type) '''
	«val generationDate = (new Date()).toString»
	/**
	 * This is the generated enum type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
	 * Generation date: «generationDate»
	 */
	(function(undefined) {

		/**
		 * @namespace «type.joynrName»
		 * @classdesc
		 * This is the generated enum type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
		 * <br/>Generation date: «generationDate»
		 «appendJSDocSummaryAndWriteSeeAndDescription(type, "* ")»
		 */
		var «type.joynrName» = {«getEnumerators(type)»};

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«type.defineName»["joynr"], function (joynr) {
				joynr.addType("«type.toTypesEnum»", «type.joynrName»);
				return «type.joynrName»;
			});
		} else if (typeof exports !== 'undefined' ) {
			if ((module !== undefined) && module.exports) {
				exports = module.exports = «type.joynrName»;
			} else {
			// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
				exports.«type.joynrName» = «type.joynrName»;
			}
			var joynr = require("joynr");
			joynr.addType("«type.toTypesEnum»", «type.joynrName»);
		} else {
			//we assume a correct order of script loading
			joynr = window.joynr;
			joynr.addType("«type.toTypesEnum»", «type.joynrName»);
			window.«type.joynrName» = «type.joynrName»;
		}
		«ELSE»
		//we assume a correct order of script loading
		window.joynr.addType("«type.toTypesEnum»", «type.joynrName»);
		window.«type.joynrName» = «type.joynrName»;
		«ENDIF»
	})();
	'''

	def getEnumerators(FEnumerationType type)'''
	«FOR enumValue: getEnumElementsAndBaseEnumElements(type) SEPARATOR ", " »
		/**
		 * @name «type.joynrName».«enumValue.joynrName»
		 * @readonly
		«IF enumValue.comment != null»
			 * @summary
			 «appendJSDocSummaryAndWriteSeeAndDescription(enumValue, "* ")»
		«ENDIF»
		 */
		«enumValue.joynrName»: «IF enumValue.value==null»"«enumValue.joynrName»"«ELSE»«enumValue.value.enumeratorValue»«ENDIF»
	«ENDFOR»
	'''
}
