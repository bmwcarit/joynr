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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.js.util.GeneratorParameter
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.util.NamingUtil
import java.util.Date
import org.franca.core.franca.FMapType

class MapTypeGenerator extends MapTemplate {

	@Inject extension JSTypeUtil
	@Inject private extension NamingUtil
	@Inject private extension JoynrJSGeneratorExtensions

	@Inject
	extension GeneratorParameter

	@Inject
	new(@Assisted FMapType type) {
		super(type)
	}

	override generate() '''
	«val generationDate = (new Date()).toString»
	/**
	 * This is the generated map type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
	 * Generation date: «generationDate»
	 */
	(function(undefined) {

		/**
		 * @namespace «type.joynrName»
		 * @classdesc
		 * This is the generated map type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
		 * <br/>Generation date: «generationDate»
		 «appendJSDocSummaryAndWriteSeeAndDescription(type, "* ")»
		 *
		 * @returns {«type.joynrName»} a new instance of a «type.joynrName»
		 */
		var «type.joynrName» = function «type.joynrName»(settings){
			if (!(this instanceof «type.joynrName»)) {
				// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
				return new «type.joynrName»(settings);
			}

			/**
			 * Used for serialization.
			 * @name «type.joynrName»#_typeName
			 * @type String
			 * @readonly
			 */
			Object.defineProperty(this, "_typeName", {
				configurable : false,
				writable : false,
				enumerable : true,
				value : "«type.joynrTypeName»"
			});

			if (settings !== undefined) {
				var clone = JSON.parse(JSON.stringify(settings)), settingKey;
				for (settingKey in clone) {
					this[settingKey] = clone[settingKey];
				}
			}

			Object.defineProperty(this, 'checkMembers', {
				enumerable: false,
				value: function checkMembers(check) {
					var memberKey;
					for (memberKey in this) {
						if (this.hasOwnProperty(memberKey)) {
							check(this[memberKey], «type.valueType.checkPropertyTypeName(false)», memberKey);
						}
					}
				}
			});

			Object.defineProperty(this, 'put', {
				enumerable: false,
				value: function (key, value) {
					this[key] = value;
				}
			});

			Object.defineProperty(this, 'get', {
				enumerable: false,
				value: function (key) {
					return this[key];
				}
			});

			Object.defineProperty(this, 'remove', {
				enumerable: false,
				value: function (key, value) {
					delete this[key];
				}
			});
		};

		«IF requireJSSupport»
		// AMD support
		if (typeof define === 'function' && define.amd) {
			define(«type.defineName»["joynr"], function (joynr) {
				«type.joynrName».prototype = new joynr.JoynrObject();
				«type.joynrName».prototype.constructor = «type.joynrName»;
				joynr.addType("«type.joynrTypeName»", «type.joynrName»);
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
			joynr.addType("«type.joynrTypeName»", «type.joynrName»);
		} else {
			//we assume a correct order of script loading
			joynr = window.joynr;
			«type.joynrName».prototype = new joynr.JoynrObject();
			«type.joynrName».prototype.constructor = «type.joynrName»;
			joynr.addType("«type.joynrTypeName»", «type.joynrName»);
			window.«type.joynrName» = «type.joynrName»;
		}
		«ELSE»
		//we assume a correct order of script loading
		«type.joynrName».prototype = new window.joynr.JoynrObject();
		«type.joynrName».prototype.constructor = «type.joynrName»;
		window.joynr.addType("«type.joynrTypeName»", «type.joynrName»);
		window.«type.joynrName» = «type.joynrName»;
		«ENDIF»
	})();
	'''
}
