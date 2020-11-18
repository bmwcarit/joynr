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
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.util.NamingUtil
import java.util.Date
import org.franca.core.franca.FMapType

class MapTypeGenerator extends MapTemplate {

	@Inject extension JSTypeUtil
	@Inject extension NamingUtil
	@Inject extension JoynrJSGeneratorExtensions

	@Inject
	new(@Assisted FMapType type) {
		super(type)
	}

	override generate(boolean generateVersion) '''
	«val generationDate = (new Date()).toString»
	«val mapType = type.valueType.tsTypeName»
	«IF !type.valueType.isPrimitive»
	import «type.valueType.tsTypeName» = require("«type.valueType.getRelativeImportPath(type, generateVersion)»")
	«ENDIF»
	import JoynrMap = require("joynr/joynr/types/JoynrMap");
	/**
	 * This is the generated map type «type.joynrName»: DOCS GENERATED FROM INTERFACE DESCRIPTION
	 * Generation date: «generationDate»
	 «appendJSDocSummaryAndWriteSeeAndDescription(type, "* ")»
	 */
	class «type.joynrName» extends JoynrMap<«mapType»> {
		public static _typeName: string = "«type.getJoynrTypeName(generateVersion)»";
		public _typeName: string = "«type.getJoynrTypeName(generateVersion)»";

		public constructor(settings?: Record<string, «mapType»>){
			super(settings);
		}

		public static checkMembers(instance: any, check: Function): void {
			for (const memberKey in instance) {
				if (Object.prototype.hasOwnProperty.call(instance, memberKey)) {
					if (memberKey !== "_typeName") {
						check(instance[memberKey], «type.valueType.checkPropertyTypeName(false)», memberKey);
					}
				}
			}
		}

		/**
		 * The MAJOR_VERSION of the map type «type.joynrName» is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MAJOR_VERSION = «majorVersion»;

		/**
		 * The MINOR_VERSION of the map type «type.joynrName» is GENERATED FROM THE INTERFACE DESCRIPTION
		 */
		public static MINOR_VERSION = «minorVersion»;
	}
	export = «type.joynrName»;

	'''
}
