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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.CompoundTypeTemplate
import io.joynr.generator.templates.util.NamingUtil
import java.util.Date
import java.util.HashSet
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FField
import org.franca.core.franca.FStructType
import org.franca.core.franca.FUnionType

class CompoundTypeGenerator extends CompoundTypeTemplate {

	@Inject extension JSTypeUtil
	@Inject extension NamingUtil
	@Inject extension JoynrJSGeneratorExtensions

	@Inject
	new(@Assisted FCompoundType type) {
		super(type)
	}

	override generate() '''
		«IF type instanceof FStructType»
			«generateStructType(type)»
		«ELSEIF type instanceof FUnionType»
			«generateUnionType(type)»
		«ENDIF»
	'''

	def generateUnionType(FUnionType type) '''
		//TODO generate union type «type.joynrName»
	'''

	def Iterable<FField> filterDuplicateTypeNames(Iterable<FField> fields){
		val set = new HashSet<String>;
		fields.filter[set.add(it.type.tsTypeName)]
	}

	def generateStructType(FStructType type) '''
	«val generationDate = (new Date()).toString»
	«val members = getMembersRecursive(type)»
	import JoynrCompound = require("joynr/joynr/types/JoynrCompound");
	«val filteredMembers = members.filterDuplicateTypeNames»
	«FOR member : filteredMembers»
	«val importPath = member.getRelativeImportPath(type)»
	«IF importPath !== null»
	import «member.type.tsTypeName» = require("«importPath»");
	«ENDIF»
	«ENDFOR»

	namespace «type.joynrName» {
		«IF members.length > 0»
			export interface «type.joynrName»Members {
				«FOR member : members»
					«member.joynrName»: «member.tsTypeName»;
				«ENDFOR»
			}
		«ELSE»
			export type «type.joynrName»Members = {} | void;
		«ENDIF»
	}
	/**
	 * This is the generated struct type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
	 * <br/>Generation date: «generationDate»
	 «appendJSDocSummaryAndWriteSeeAndDescription(type, "* ")»
	 */
	class «type.joynrName» extends JoynrCompound {
		public static _typeName: string = "«type.joynrTypeName»";
		public _typeName: string = "«type.joynrTypeName»";

		«FOR member : members»
		«IF member.comment !== null»
		/**
		 * «FOR comment : member.comment.elements»«comment.
		    comment.replaceAll("\n", "\n" + "* ")»«ENDFOR»
		 */
		 «ENDIF»
		public «member.joynrName»!: «member.tsTypeName»;
		«ENDFOR»

		/**
		 * @param members - an object containing the individual member elements
		 «FOR member : members»
		 * @param members.«member.joynrName» - «IF member.comment !== null»«FOR comment : member.comment.elements»«comment.
			comment.replaceAll("\n", "\n" + "* ")»«ENDFOR»«ENDIF»
		 «ENDFOR»
		 */
		public constructor(members: «type.joynrName».«type.joynrName»Members) {
			super();
			«IF type.base !== null»
			/**
			 * Parent class.
			 * @name «type.joynrName»#_extends
			 * @type String
			 * @readonly
			 */
			Object.defineProperty(this, "_extends", {
				value : "«type.base.joynrTypeName»"
			});
			«ENDIF»
			if (members) {
			«FOR member : members»
				this.«member.joynrName» = members.«member.joynrName»;
			«ENDFOR»
			}
		}

		public static checkMembers(_instance: «type.joynrName», _check: Function): void {
			«FOR member : members»
				_check(_instance.«member.joynrName», «member.checkPropertyTypeName», "members.«member.joynrName»");
			«ENDFOR»
		}

		/**
		 * The MAJOR_VERSION of the struct type «type.joynrName» is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MAJOR_VERSION = «majorVersion»;

		/**
		 * The MINOR_VERSION of the struct type «type.joynrName» is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MINOR_VERSION = «minorVersion»;

		public static readonly _memberTypes: Record<string, string> = {
			«FOR member : members SEPARATOR ","»
				«member.joynrName»: "«member.joynrTypeName»"
			«ENDFOR»
		};

		public static getMemberType(memberName: string): string | undefined {
			if («type.joynrName»._memberTypes[memberName] !== undefined) {
				return «type.joynrName»._memberTypes[memberName];
			}
		}
	}
	export = «type.joynrName»;
	'''
}
