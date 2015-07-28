package io.joynr.generator.interfaces
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
import io.joynr.generator.util.TemplateBase
import java.util.HashMap
import java.util.ArrayList
import java.util.Collection
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FArgument
import io.joynr.generator.util.JoynrJavaGeneratorExtensions
import io.joynr.generator.util.InterfaceTemplate
import io.joynr.generator.util.JavaTypeUtil

class InterfacesTemplate implements InterfaceTemplate{
	@Inject extension JoynrJavaGeneratorExtensions
	@Inject extension JavaTypeUtil
	@Inject extension TemplateBase

	def init(FInterface serviceInterface, HashMap<FMethod, String> methodToErrorEnumName) {
		var uniqueMethodSignatureToErrorEnumName = new HashMap<String, String>();
		var methodNameToCount = overloadedMethodCounts(getMethods(serviceInterface));
		var methodNameToIndex = new HashMap<String, Integer>();

		for (FMethod method : getMethods(serviceInterface)) {
			if (methodNameToCount.get(method.name) == 1) {
				// method not overloaded, so no index needed
				methodToErrorEnumName.put(method, method.name.toFirstUpper + "ErrorEnum");
			} else {
				// initialize index if not existent
				if (!methodNameToIndex.containsKey(method.name)) {
					methodNameToIndex.put(method.name, 0);
				}
				val methodSignature = createMethodSignature(method);
				if (!uniqueMethodSignatureToErrorEnumName.containsKey(methodSignature)) {
					var Integer index = methodNameToIndex.get(method.name);
					index++;
					methodNameToIndex.put(method.name, index);
					uniqueMethodSignatureToErrorEnumName.put(methodSignature, method.name.toFirstUpper + index);
				}
				methodToErrorEnumName.put(method, uniqueMethodSignatureToErrorEnumName.get(methodSignature) + "ErrorEnum");
			}
		}
	}

	/**
	 * @return a method signature that is unique in terms of method name, in
	 *      parameter names and in parameter types.
	 */
	def createMethodSignature(FMethod method) {
		val nameStringBuilder = new StringBuilder(method.name);
		for (FArgument inParam : method.inputParameters) {
			nameStringBuilder.append(inParam.name.toFirstUpper);
			val typeName = new StringBuilder(inParam.typeName.objectDataTypeForPlainType);
			nameStringBuilder.append(typeName.toString());
		}
		return nameStringBuilder.toString;
	}

	override generate(FInterface serviceInterface) {
		val interfaceName =  serviceInterface.joynrName
		val className = interfaceName
		val packagePath = getPackagePathWithJoynrPrefix(serviceInterface, ".")
		val hasMethodWithImplicitErrorEnum = hasMethodWithImplicitErrorEnum(serviceInterface)
		var methodToErrorEnumName = new HashMap<FMethod, String>()
		init(serviceInterface, methodToErrorEnumName)
		'''

		«warning()»
package «packagePath»;

import java.util.List;
«IF hasMethodWithImplicitErrorEnum»
	import java.util.HashMap;
	import java.util.Map;
	import java.util.Map.Entry;
«ENDIF»

import com.fasterxml.jackson.core.type.TypeReference;
«FOR datatype: getRequiredIncludesFor(serviceInterface)»
	import «datatype»;
«ENDFOR»

//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used. 
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@SuppressWarnings("unused")
public interface «className»  {
	public static String INTERFACE_NAME = "«getPackagePathWithoutJoynrPrefix(serviceInterface, "/")»/«interfaceName.toLowerCase»";

	«FOR type : filterTypesByToken(getAllTypes(serviceInterface))»
		public static class «if (type.joynrNameQt==null) "Typename not found" else getTokenTypeForArrayType(type.joynrNameQt)»Token extends TypeReference<«getTokenTypeForArrayType(type.joynrNameQt)»> {}
		public static class List«getTokenTypeForArrayType(type.joynrNameQt)»Token extends TypeReference<List<«getTokenTypeForArrayType(type.joynrNameQt)»> > {}
	«ENDFOR»

	«FOR method: getMethods(serviceInterface)»
		«var enumType = method.errors»
		«IF enumType != null»
			«enumType.name = methodToErrorEnumName.get(method)»
			«generateEnumCode(enumType)»

		«ENDIF»
	«ENDFOR»
}

'''
	}

	def filterTypesByToken(Collection<Object> objects) {
		val result = new ArrayList<Object>()
		val tokens = new ArrayList<String>();
		for (object: objects){
			if (object!=null && !tokens.contains(getTokenTypeForArrayType(object.joynrNameQt))){
				result.add(object)
				tokens.add(getTokenTypeForArrayType(object.joynrNameQt))
			}
		}
		return result;
	}
}