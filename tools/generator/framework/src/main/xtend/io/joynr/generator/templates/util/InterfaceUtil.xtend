package io.joynr.generator.templates.util
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

import com.google.common.collect.Iterables
import com.google.inject.Inject
import com.google.inject.Singleton
import java.util.ArrayList
import java.util.HashMap
import java.util.HashSet
import java.util.Set
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FMethod

@Singleton
public class InterfaceUtil {

	@Inject
	private extension NamingUtil

	@Inject
	private extension TypeUtil

	@Inject
	private extension MethodUtil

	@Inject
	private extension BroadcastUtil

	@Inject
	private extension AttributeUtil

	def getMethodNames(FInterface fInterface) {
		var names = new HashSet<String>();
		for (method : fInterface.getMethods()){
			if (!names.contains(method.joynrName)){
				names.add(method.joynrName)
			}
		}
		return names;
	}

	def getMethods(FInterface fInterface, String methodName){
		var result = new ArrayList<FMethod>();
		for (method : fInterface.getMethods()){
			if (method.joynrName!=null && method.joynrName.equals(methodName)){
				result.add(method)
			}
		}
		return result;
	}

	def getMethods(FInterface fInterface) {
		fInterface.methods
	}

	def getEvents(FInterface fInterface) {
		fInterface.broadcasts
	}

	def getUniqueMethodNames(FInterface fInterface) {
		return getUniqueMethodNames(getMethods(fInterface));
	}

	def getUniqueMethodNames(Iterable<FMethod> methods) {
		val set = new HashSet<String>()
		for (method : methods) {
			set.add(method.joynrName);
		}
		return set;
	}

	def getAttributes(FInterface fInterface) {
		fInterface.attributes
	}

	def hasArray (FInterface fInterface) {
		for (method : fInterface.methods) {
			for (args : Iterables::concat(method.inputParameters, method.outputParameters)) {
				if (isArray(args)) {
					return true
				}
			}
		}

		for (broadcast : fInterface.broadcasts) {
			for (args : broadcast.outputParameters) {
				if (isArray(args)) {
					return true
				}
			}
		}

		for (attribute : getAttributes(fInterface)) {
			if (isArray(attribute)) {
				return true
			}
		}
		return false
	}

	def getAllTypes(FInterface fInterface) {
		val typeList = new HashMap<String, Object>()
		for (method : getMethods(fInterface)) {
			for (returnParameter : getOutputParameters(method)) {
				if (returnParameter != null) {
					val datatype = getDatatype(returnParameter.type)
					val typename = datatype.joynrName;
					if (typename != null){
						if (!typeList.containsKey(typename)){
							typeList.put(datatype.joynrName, datatype);
						}
					}
					else {
						throw new IllegalStateException ("Typename for output parameter " + returnParameter.joynrName + " of method " + fInterface.joynrName + "." + method.joynrName + " could not be resolved")
					}
				}
			}

			for (inputParameter : getInputParameters(method)) {
				if (inputParameter != null) {
					val datatype = getDatatype(inputParameter.type)
					val typename = datatype.joynrName;
					if (typename != null){
						if (!typeList.containsKey(typename)){
							typeList.put(datatype.joynrName, datatype);
						}
					}
					else {
						throw new IllegalStateException ("Typename for input parameter " + inputParameter.joynrName + " of method " + fInterface.joynrName + "." + method.joynrName + " could not be resolved")
					}
				}
			}
		}
		for (attribute : getAttributes(fInterface)) {
			val datatype = getDatatype(attribute.type)
			if (!typeList.containsKey(datatype.joynrName)){
				typeList.put(datatype.joynrName, datatype);
			}
		}
		return typeList.values;
	}

	def getAllRequiredTypes(
		FInterface fInterface
	) {
		fInterface.getAllRequiredTypes(TypeSelector::defaultTypeSelector)
	}

	def addTypesFromMethod(FInterface fInterface,
		boolean fireAndForget,
		boolean errorTypes,
		Set<Object> typeList
	) {
		val methodToErrorEnumName = fInterface.methodToErrorEnumName
		for (method : fInterface.methods.filter[method | method.fireAndForget == fireAndForget]) {
			typeList.addAll(getAllRequiredTypes(method, methodToErrorEnumName.get(method), errorTypes))
		}
	}

	def getAllRequiredTypes(
			FInterface fInterface,
			TypeSelector selector
	) {
		val typeList = new HashSet<Object>();
		if (selector.methods){
			addTypesFromMethod(fInterface, false, selector.errorTypes, typeList)
		}
		if (selector.fireAndForget) {
			addTypesFromMethod(fInterface, true, selector.errorTypes, typeList)
		}

		for (attribute : getAttributes(fInterface)) {
			if ((selector.readAttributes && attribute.readable)
					|| (selector.writeAttributes && attribute.writable)
					|| (selector.notifyAttributes && attribute.notifiable)
			) {
				typeList.addAll(getRequiredTypes(attribute.type));
			}
		}

		if (selector.broadcasts) {
			for (broadcast : fInterface.broadcasts) {
				typeList.addAll(getAllRequiredTypes(broadcast))
			}
		}

		if (selector.transitiveTypes){
			var returnValue = new HashSet<Object>()
			getAllReferredDatatypes(typeList, returnValue)
			return returnValue
		}
		else{
			return typeList;
		}
	}

	def getAllComplexTypes(
			FInterface fInterface
	) {
		getAllComplexTypes(fInterface, TypeSelector::defaultTypeSelector)
	}

	def getAllComplexTypes(
			FInterface fInterface,
			TypeSelector selector
	) {
		getAllRequiredTypes(fInterface, selector).filterComplex(selector.typeDefs)
	}

	def private void getAllReferredDatatypes(Iterable<Object> list, HashSet<Object> cache) {
		for(element : list){
			if (!cache.contains(element)){
				cache.add(element)
				if (element instanceof FCompoundType){
					getAllReferredDatatypes(element.members.map[e | e.type.datatype], cache)
				}
				if (element instanceof FMapType){
					getAllReferredDatatypes(newArrayList(element.keyType, element.valueType), cache)
				}
			}
		}
	}

	static class TypeSelector {
		var methods = true
		var fireAndForget = true
		var readAttributes = true
		var writeAttributes = true
		var notifyAttributes = true
		var broadcasts = true
		var errorTypes = false
		var transitiveTypes = false
		var typeDefs = false
		static def defaultTypeSelector () {
			new TypeSelector()
		}

		def methods(boolean methods) {
			this.methods = methods
		}

		def fireAndForget(boolean fireAndForget) {
		    this.fireAndForget = fireAndForget
		}

		def readAttributes(boolean readAttributes) {
			this.readAttributes = readAttributes
		}

		def writeAttributes(boolean writeAttributes) {
			this.writeAttributes = writeAttributes
		}

		def notifyAttributes(boolean notifyAttributes) {
			this.notifyAttributes = notifyAttributes
		}

		def broadcasts(boolean broadcasts) {
			this.broadcasts = broadcasts
		}

		def errorTypes(boolean errorTypes) {
			this.errorTypes = errorTypes
		}

		def transitiveTypes(boolean transitiveTypes) {
			this.transitiveTypes = transitiveTypes
		}

		def typeDefs(boolean typeDefs) {
			this.typeDefs = typeDefs
		}
	}

	def boolean hasReadAttribute(FInterface interfaceType){
		return interfaceType.attributes.exists[readable]
	}

	def boolean hasWriteAttribute(FInterface interfaceType){
		return interfaceType.attributes.exists[writable]
	}

	def boolean hasNotifiableAttribute(FInterface interfaceType){
		return interfaceType.attributes.exists[notifiable]
	}

	def boolean hasMethodWithReturnValue(FInterface interfaceType){
		for(method: interfaceType.methods){
			if (!method.outputParameters.empty){
				return true
			}
		}
		return false
	}

	def boolean hasMethodWithoutReturnValue(FInterface interfaceType) {
		for (method: interfaceType.methods) {
			if (method.outputParameters.empty) {
				return true
			}
		}
		return false
	}

	def boolean hasMethodWithImplicitErrorEnum(FInterface interfaceType){
		for(method: interfaceType.methods){
			if (method.errors != null) {
				return true
			}
		}
		return false
	}

	def boolean hasMethodWithErrorEnum(FInterface interfaceType) {
		for (method : interfaceType.methods) {
			if (method.errorEnum != null) {
				return true;
			}
		}
		return hasMethodWithImplicitErrorEnum(interfaceType);
	}

	def boolean hasMethodWithEnumInputParameter(FInterface interfaceType) {
		for (method : interfaceType.methods) {
			if (method.hasEnumInputParameter) {
				return true;
			}
		}
		return false;
	}

	def boolean hasMethodWithArguments(FInterface interfaceType){
		for(method: interfaceType.methods){
			if (getInputParameters(method).size>0){
				return true
			}
		}
		return false
	}

	def getAllPrimitiveTypes(FInterface serviceInterface) {
		serviceInterface.allRequiredTypes.filter[type | type instanceof FBasicTypeId].map[type | type as FBasicTypeId]
	}

	def methodToErrorEnumName(FInterface serviceInterface) {
		var HashMap<FMethod, String> methodToErrorEnumName = new HashMap<FMethod, String>()
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
				val methodSignature = createMethodSignatureFromInParameters(method);
				if (!uniqueMethodSignatureToErrorEnumName.containsKey(methodSignature)) {
					var Integer index = methodNameToIndex.get(method.name);
					index++;
					methodNameToIndex.put(method.name, index);
					uniqueMethodSignatureToErrorEnumName.put(methodSignature, method.name.toFirstUpper + index);
				}
				methodToErrorEnumName.put(method, uniqueMethodSignatureToErrorEnumName.get(methodSignature) + "ErrorEnum");
			}
		}
		return methodToErrorEnumName
	}

	def hasNonFireAndForgetMethods(FInterface francaInterface) {
		francaInterface.methods.exists[!fireAndForget]
	}

	def hasFireAndForgetMethods(FInterface francaInterface) {
		francaInterface.methods.exists[fireAndForget]
	}
}
