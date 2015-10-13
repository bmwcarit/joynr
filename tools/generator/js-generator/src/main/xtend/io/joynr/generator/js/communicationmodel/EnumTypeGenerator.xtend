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
		var «type.joynrName» = function «type.joynrName»(settings){
			if (!(this instanceof «type.joynrName»)) {
				// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
				return new «type.joynrName»(members);
			}

			if (settings !== undefined) {
				this.name = settings.name;
				this.value = settings.value;
			}

			/**
			 * Used for serialization.
			 * @name «type.joynrName»#_typeName
			 * @type String
			 * @field
			 * @readonly
			 */
			Object.defineProperty(this, "_typeName", {
				configurable : false,
				writable : false,
				enumerable : true,
				value : "«type.toTypesEnum»"
			});

		};

		var createLiterals = function() {
			«getEnumerators(type)»
		};

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«type.defineName»["joynr"], function (joynr) {
				«type.joynrName».prototype = new joynr.JoynrObject();
				«type.joynrName».prototype.constructor = «type.joynrName»;
				createLiterals();
				joynr.addType("«type.toTypesEnum»", «type.joynrName», true);
				return «type.joynrName»;
			});
		} else if (typeof exports !== 'undefined' ) {
			if ((module !== undefined) && module.exports) {
				exports = module.exports = «type.joynrName»;
			} else {
			// support CommonJS module 1.1.1 spec (`exports` cannot be a function)
				exports.«type.joynrName» = «type.joynrName»;
			}
			var joynr = requirejs("joynr");
			«type.joynrName».prototype = new joynr.JoynrObject();
			«type.joynrName».prototype.constructor = «type.joynrName»;
			createLiterals();
			joynr.addType("«type.toTypesEnum»", «type.joynrName», true);
		} else {
			//we assume a correct order of script loading
			joynr = window.joynr;
			«type.joynrName».prototype = new joynr.JoynrObject();
			«type.joynrName».prototype.constructor = «type.joynrName»;
			createLiterals();
			joynr.addType("«type.toTypesEnum»", «type.joynrName», true);
			window.«type.joynrName» = «type.joynrName»;
		}
		«ELSE»
		//we assume a correct order of script loading
		«type.joynrName».prototype = new window.joynr.JoynrObject();
		«type.joynrName».prototype.constructor = «type.joynrName»;
		createLiterals();
		window.joynr.addType("«type.toTypesEnum»", «type.joynrName», true);
		window.«type.joynrName» = «type.joynrName»;
		«ENDIF»
	})();
	'''

	private def getEnumerators(FEnumerationType type)'''
	«FOR literal: type.enumerators»
		/**
		 * @name «type.joynrName».«literal.joynrName»
		 * @readonly
		 «IF literal.comment != null»
		 	* @summary
		 	«appendJSDocSummaryAndWriteSeeAndDescription(literal, "* ")»
		 «ENDIF»
		 */
		«type.joynrName».«literal.joynrName» = new «type.joynrName»({
			name: "«literal.joynrName»",
			value: «IF literal.value==null»"«literal.joynrName»"«ELSE»«literal.value.enumeratorValue»«ENDIF»
		});
	«ENDFOR»
	'''

}
