package io.joynr.generator.js.provider

/*
 * !!!
 *
 * Copyright (C) 2022 BMW Car IT GmbH
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
import io.joynr.generator.js.templates.InterfaceJsTemplate
import io.joynr.generator.js.util.JSTypeUtil
import io.joynr.generator.js.util.JoynrJSGeneratorExtensions
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.util.Date
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FType
import io.joynr.generator.templates.util.AttributeUtil

class ProviderImplCreatorGenerator extends InterfaceJsTemplate {

	@Inject extension JoynrJSGeneratorExtensions
	@Inject extension JSTypeUtil
	@Inject extension AttributeUtil
	@Inject extension NamingUtil
	@Inject extension MethodUtil
	@Inject extension InterfaceUtil

	def relativePathToBase() {
		var relativePath = ""
		for (var i=0; i<packagePathDepth; i++) {
			relativePath += "../"
		}
		return relativePath
	}

	def generateProvider(IFileSystemAccess fsa, boolean generateVersion){
		super.generate(generateVersion)
		var fileName = path + "" + providerName + "ImplementationCreator.ts"
		if (clean) {
			fsa.deleteFile(fileName)
		}
		if (generate) {
			fsa.generateFile(
				fileName,
				generate(generateVersion).toString
			)
		}
	}

	def getProviderName(){
		francaIntf.joynrName + "Provider"
	}

	def attributesContainDatatype(FType type) {
		val attributes = getAttributes(francaIntf)
		for(attribute : attributes){
			// Substring to cover the cases where we have an array as attribute, since then tsTypeName returns
			// '[]' at the end, which makes the straightforward comparison with the datatype fail
			val attributeNameTrimmed = attribute.tsTypeName.substring(0, attribute.tsTypeName.length() - 2);
			if(attribute.tsTypeName == type.tsTypeName || attributeNameTrimmed == type.tsTypeName) {
				return true
			}
		}
		return false
	}

	override generate(boolean generateVersion) {
		val generationDate = (new Date()).toString
		val attributes = getAttributes(francaIntf)
		val methodNames = getMethodNames(francaIntf)
		'''
«FOR datatype : francaIntf.getAllComplexTypes(typeSelectorIncludingErrorTypesAndTransitiveTypes)»
	«IF attributesContainDatatype(datatype)»
		import «datatype.joynrName» = require("«relativePathToBase() + datatype.getDependencyPath(generateVersion)»");
	«ENDIF»
«ENDFOR»
import * as «providerName» from "./«providerName»"

/**
 * PLEASE NOTE. THIS IS A GENERATED FILE!
 * <br/>Generation date: «generationDate»
 *
 * This is a class to create a very basic provider implementation for the interface
 * «providerName»Implementation that can be used for testing.
 * It implements every function (methods as well as attribute getters and setters) to return
 * either void, or an empty object depending on the specification.
 * The return values are set as attributes <functionName>ReturnValue in the created implementation.
 * This allows overwriting the return values (if not void) to change the behavior of the provider.
 *
 * NOTE:
 * The generated code does not fully support interfaces with overloaded methods.
 * Different return types cannot be distinguished. All overloads of a method will return the same value.
 */
class «providerName.toFirstUpper»ImplementationCreator {
	// attributes
	«FOR attribute: attributes»
		«val attributeName = attribute.joynrName»
		«IF isReadable(attribute)»
			public «attributeName»ReturnValue = {} as «attribute.tsTypeName»;
			public «attributeName»: any = {
				get(): «attribute.tsTypeName» {
					console.log(`Get called for attribute «francaIntf.joynrName».«attributeName». Response: ${JSON.stringify(this.«attributeName»ReturnValue)}`);
					return this.«attributeName»ReturnValue;
				}«IF isWritable(attribute)»,«ENDIF»
				«IF isWritable(attribute)»
					set(value: «attribute.tsTypeName»): void {
						console.log(`Set called for attribute «francaIntf.joynrName».«attributeName». Setting: ${JSON.stringify(value)}`);
						this.«attributeName»ReturnValue = value;
					}
				«ENDIF»
			}
		«ENDIF»

	«ENDFOR»
	// methods
	«FOR operationName : methodNames»
		«val operations = getMethods(francaIntf, operationName)»
		«val operation = operations.get(0)»
		«IF getOutputParameters(operation).length > 0»
			public «operationName»ReturnValue = {} as «FOR i : 1..operations.size SEPARATOR "&"»«providerName».«operationName.toFirstUpper»Returns«i»«ENDFOR»;
		«ELSE»
			public «operationName»ReturnValue = undefined as «FOR i : 1..operations.size SEPARATOR "&"»«providerName».«operationName.toFirstUpper»Returns«i»«ENDFOR»;
		«ENDIF»
		public _«operationName»(_: «FOR i : 1..operations.size SEPARATOR "|"»«providerName».«operationName.toFirstUpper»Args«i»«ENDFOR»): «FOR i : 1..operations.size SEPARATOR "&"»«providerName».«operationName.toFirstUpper»Returns«i»«ENDFOR» {
			console.log(`Method «francaIntf.joynrName».«operationName» called. Response: ${JSON.stringify(this.«operationName»ReturnValue)}`)
			return this.«operationName»ReturnValue;
		}

	«ENDFOR»
	public getProviderImplementation(): «providerName».«providerName»Implementation {
		«FOR attribute: attributes»
			«val attributeName = attribute.joynrName»
			«IF isReadable(attribute)»
				this.«attributeName».get = this.«attributeName».get.bind(this);
			«ENDIF»
			«IF isWritable(attribute)»
				this.«attributeName».set = this.«attributeName».set.bind(this);
			«ENDIF»
		«ENDFOR»

		return {
			«IF attributes.length > 0»
				«FOR operationName : methodNames »
					«operationName»: this._«operationName».bind(this),
				«ENDFOR»
			«ELSE»
				«FOR operationName : methodNames SEPARATOR ","»
					«operationName»: this._«operationName».bind(this)
				«ENDFOR»
			«ENDIF»
			«FOR attribute: attributes  SEPARATOR  ","»
				«val attributeName = attribute.joynrName»
				«IF isReadable(attribute)»
					«attributeName»: this.«attributeName»
				«ELSE»
					«attributeName»: {} as «attribute.providerAttributeImplName»<«attribute.tsTypeName»>
				«ENDIF»
			«ENDFOR»
		}
	}
}

export = «providerName.toFirstUpper»ImplementationCreator;
		'''
	}
}
