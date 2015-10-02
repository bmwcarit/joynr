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
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import java.util.Set
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FStructType
import org.franca.core.franca.FUnionType
import java.util.Date
import org.franca.core.franca.FField
import io.joynr.generator.js.util.GeneratorParameter
import io.joynr.generator.js.util.JSTypeUtil

class CompoundTypeGenerator {

	@Inject
	extension JoynrJSGeneratorExtensions

	@Inject
	extension JSTypeUtil

	@Inject
	extension GeneratorParameter

	def generateCompoundType(FCompoundType compoundType, Set<Object> generatedTypes) '''
		«IF !generatedTypes.contains(compoundType)»
			«IF compoundType instanceof FStructType»
				«generateStructType(compoundType)»
			«ELSEIF compoundType instanceof FUnionType»
				«generateUnionType(compoundType as FUnionType)»
			«ENDIF»
			«generatedTypes.updateGeneratedTypesSet(compoundType)»
		«ENDIF»
	'''

	def private void updateGeneratedTypesSet(Set<Object> generatedTypes, Object newType) {
		generatedTypes.add(newType)
	}

	def generateUnionType(FUnionType type) '''
		//TODO generate union type «type.joynrName»
	'''

	def checkPropertyTypeName(FField field) {
		if (field.isArray) {
			return "\"Array\""
		}
		if (field.type.isPrimitive) {
			if (field.type.getPrimitive.bool) {
				return "\"Boolean\""
			}
			if (field.type.getPrimitive.string) {
				return "\"String\""
			}
			return "\"Number\""
		} else {
			if (field.type.complex) {
				return "\"" + field.type.derived.joynrName + "\""
			}
			else {
				/* TODO in the final version, enumerations must always be represented as object.
				 * Thus, String must be removed here once enums are fully supported
				 */
				return  "[\"String\", \"Object\", \"" + field.type.derived.joynrName + "\"]" 
			}
		}
	}

	def generateStructType(FStructType type) '''
		«val generationDate = (new Date()).toString»
		/**
		 * This is the generated struct type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
		 * Generation date: «generationDate»
		 */
		(function(undefined) {
			/**
			 * @name «type.joynrName»
			 * @constructor
			 *
			 * @classdesc
			 * This is the generated struct type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
			 * <br/>Generation date: «generationDate»
			 «appendJSDocSummaryAndWriteSeeAndDescription(type, "* ")»
			 *
			 * @param {Object} members - an object containing the individual member elements
			 «val members = getMembersRecursive(type)»
			 «FOR member : members»
			 * @param {«member.jsdocTypeName»} members.«member.joynrName» - «IF member.comment != null»«FOR comment : member.comment.elements»«comment.
				comment.replaceAll("\n", "\n" + "* ")»«ENDFOR»«ENDIF»
			 «ENDFOR»
			 * @returns {«type.joynrName»} a new instance of a «type.joynrName»
			 */
			var «type.joynrName» = function «type.joynrName»(members) {
				if (!(this instanceof «type.joynrName»)) {
					// in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
					return new «type.joynrName»(members);
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
				«IF type.base != null»

				/**
				 * Parent class.
				 * @name «type.joynrName»#_extends
				 * @type String
				 * @field
				 * @readonly
				 */
				Object.defineProperty(this, "_extends", {
					configurable : false,
					writable : false,
					enumerable : false,
					value : "«type.base.toTypesEnum»"
				});
				«ENDIF»

				«FOR member : members»
					/**
					 * «IF member.comment != null»«FOR comment : member.comment.elements»«comment.
						comment.replaceAll("\n", "\n" + "* ")»«ENDFOR»«ENDIF»
					 * @name «type.joynrName»#«member.joynrName»
					 * @type «member.jsdocTypeName»
					 * @field
					 */
				«ENDFOR»
				Object.defineProperty(this, 'checkMembers', {
					enumerable: false,
					value: function checkMembers(check) {
						«FOR member : members»
						check(this.«member.joynrName», «member.checkPropertyTypeName», "members.«member.joynrName»");
						«ENDFOR»
					}
				});

				if (members !== undefined) {
					«FOR member : members»
					this.«member.joynrName» = members.«member.joynrName»;
					«ENDFOR»
				}

			};

			var memberTypes = {
				«FOR member : members SEPARATOR ","»
				«member.joynrName»: "«member.type.toTypesEnum»"
				«ENDFOR»
			};
			Object.defineProperty(«type.joynrName», 'getMemberType', {
				enumerable: false,
				value: function getMemberType(memberName) {
					return memberTypes[memberName];
				}
			});

			«IF requireJSSupport»
			// AMD support
			if (typeof define === 'function' && define.amd) {
				define(«type.defineName»["joynr"], function (joynr) {
					«type.joynrName».prototype = new joynr.JoynrObject();
					«type.joynrName».prototype.constructor = «type.joynrName»;
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
				var joynr = requirejs("joynr");
				«type.joynrName».prototype = new joynr.JoynrObject();
				«type.joynrName».prototype.constructor = «type.joynrName»;

				joynr.addType("«type.toTypesEnum»", «type.joynrName»);
			} else {
				«type.joynrName».prototype = new window.joynr.JoynrObject();
				«type.joynrName».prototype.constructor = «type.joynrName»;
				window.joynr.addType("«type.toTypesEnum»", «type.joynrName»);
				window.«type.joynrName» = «type.joynrName»;
			}
			«ELSE»
				//we assume a correct order of script loading
			«type.joynrName».prototype = new window.joynr.JoynrObject();
			«type.joynrName».prototype.constructor = «type.joynrName»;
			window.joynr.addType("«type.toTypesEnum»", «type.joynrName»);
			window.«type.joynrName» = «type.joynrName»;
			«ENDIF»
		})();
	'''
}
