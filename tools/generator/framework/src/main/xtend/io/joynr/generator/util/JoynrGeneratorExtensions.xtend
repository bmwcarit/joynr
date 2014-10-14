package io.joynr.generator.util
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import com.google.common.collect.Iterators
import java.io.BufferedReader
import java.io.IOException
import java.io.StringReader
import java.util.ArrayList
import java.util.Arrays
import java.util.HashMap
import java.util.HashSet
import org.eclipse.emf.ecore.impl.BasicEObjectImpl
import org.franca.core.franca.FArgument
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FField
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModel
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FUnionType
import org.franca.core.franca.FEnumerator
import org.franca.core.franca.FAnnotation
import java.util.List
import org.franca.core.franca.FAnnotationType

abstract class JoynrGeneratorExtensions {

	def Iterable<FInterface> getInterfaces(FModel model) {
		return model.interfaces
	}

	def Iterable<FArgument> getOutputParameters(FMethod method) {
		if (method == null || method.outArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			// method.getOutArgs().filterNull
			Arrays::asList(method.getOutArgs().filterNull.head)
		}
	}

	def Iterable<FArgument> getOutputParameters(FBroadcast event) {
		if (event == null || event.outArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			event.outArgs.filterNull
		}
	}

	def Iterable<FArgument> getInputParameters(FMethod method) {
		if (method == null || method.inArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			return method.inArgs.filterNull
		}
	}

	// Convert an collection of output parameters to their typenames
	def Iterable<String> mapOutputParameters(Iterable<FArgument> parameters) {
		val result = new HashSet<String>();
		if (parameters.empty) {
			result.add("void");
		} else {
			for (FArgument parameter : parameters) {
				result.add(getMappedDatatypeOrList(parameter))
			}
		}
		return result;
	}

	def Iterable<String> getMappedOutputParameter(FMethod method) {
		val result = new HashSet<String>();
		val types = getOutputParameters(method);
		if (types.empty) {
			result.add("void");
		}
		for (FArgument argument : types) {
			result.add(getMappedDatatypeOrList(argument));
		}
		return result;
	}

	def String getPackageNameInternal(FModelElement fModelElement, boolean useOwnName) {
		if (fModelElement == null){
			throw new IllegalStateException("Generator could not proceed with code generation, since JoynGeneratorExtensions.getPackageNameInternal has been invoked with an empty model element");
		} else if (fModelElement.eContainer == null){
			val errorMsg = "Generator could not proceed with code generation, since " +
							if (fModelElement.joynrName != null)
								"the container of model element " + fModelElement.joynrName + " is not known" else "the resource " +
							(if (fModelElement instanceof BasicEObjectImpl)
								 (fModelElement as BasicEObjectImpl).eProxyURI
							else
								fModelElement.eResource.toString) + " cannot be parsed correctly"
			throw new IllegalStateException(errorMsg);
		} else if (fModelElement.eContainer instanceof FModel)
			return (fModelElement.eContainer as FModel).joynrName
		return (fModelElement.eContainer as FModelElement).getPackageNameInternal(true) + (if (useOwnName) '.' + fModelElement.joynrName else '')
	}

	def getPackageName(FModelElement fModelElement) {
		getPackageNameInternal(fModelElement, false)
	}

	def getPackageNames(FModelElement fModelElement) {
		getPackageNames(fModelElement, "\\.")
	}

	def getPackageNames(FModelElement fModelElement, String separator) {
		Iterators::forArray(fModelElement.packageName.split(separator))
	}

	def getPackagePathWithJoynrPrefix(FModelElement fModelElement, String separator) {
		return joynrGenerationPrefix + separator + getPackageName(fModelElement).replace('.', separator)
		// Get the package path but remove the leading namespace
	}

	def getPackagePathWithoutJoynrPrefix(FModelElement fModelElement, String separator) {
		return getPackageName(fModelElement).replace('.', separator)
	}

	def String getMappedDatatype(FTypedElement typedElement){
		getMappedDatatype(typedElement.type)
	}

	def String getMappedDatatype(FTypeRef type){
		if (type.derived != null){
			getMappedDatatype(type.derived)
		}
		else{
			getPrimitiveTypeName(type.predefined)
		}
	}

	def String getPrimitiveTypeName(FBasicTypeId basicType)

	def String getMappedDatatypeOrList(FTypedElement typedElement){
		if (typedElement.type.derived != null){
			var result = getMappedDatatypeOrList(typedElement.type.derived, typedElement.array == '[]')
			if (result == null){
				throw new IllegalStateException ("Datatype for element " + typedElement.joynrName + " could not be found");
			}
			return result;
		}
		else{
			var result = getMappedDatatypeOrList(typedElement.type.predefined, typedElement.array == '[]')
			if (result == null){
				throw new IllegalStateException ("Datatype for element " + typedElement.joynrName + " could not be found");
			}
			return result;
		}
	}

	def String getMappedDatatype(FType datatype)

	def String getMappedDatatypeOrList(FType datatype, boolean array)

	def String getMappedDatatypeOrList(FBasicTypeId datatype, boolean array)

	def String getMappedOutputParameterTypesCommaSeparated(FBroadcast broadcast) {
		val commaSeparatedParams = new StringBuilder();
		for (parameter : mapOutputParameters(getOutputParameters(broadcast))) {
			commaSeparatedParams.append(parameter);
			commaSeparatedParams.append(", ");
		}
		val returnString = commaSeparatedParams.toString();
		if (returnString.length() == 0) {
			return "";
		}
		else{
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	def String getMappedOutputParametersCommaSeparated(FBroadcast broadcast, boolean constRef) {
		val commaSeparatedParams = new StringBuilder();
		for (parameter : getOutputParameters(broadcast)) {
			commaSeparatedParams.append("\n        ")
			if (constRef) {
				commaSeparatedParams.append("const ")
			}
			commaSeparatedParams.append(getMappedDatatypeOrList(parameter));
			if (constRef) {
				commaSeparatedParams.append("& ")
			}
			commaSeparatedParams.append(parameter.name);
			commaSeparatedParams.append(",");
		}
		val returnString = commaSeparatedParams.toString();
		if (returnString.length() == 0) {
			return "";
		}
		else{
			return returnString.substring(0, returnString.length() - 1); //remove the last ","
		}
	}

	def getMethods(FInterface fInterface) {
		fInterface.methods
	}

	def getEvents(FInterface fInterface) {
		fInterface.broadcasts
	}

	def getUniqueMethodNames(FInterface fInterface) {
		val set = new HashSet<String>()
		for (method : getMethods(fInterface)) {
			set.add(method.joynrName);
		}
		return set;
	}

	def getAttributes(FInterface fInterface) {
		fInterface.attributes
	}

	def getFilterParameters(FBroadcast broadcast) {
		val paramList = new ArrayList<String>();
		if (broadcast.comment != null) {
			for(annotation: broadcast.comment.elements) {
				if(annotation.type == FAnnotationType::PARAM) {
					val comment = annotation.comment
					paramList.add(comment.split("\\s+").get(0))
				}
			}
		}
		return paramList
	}

	def getAllComplexAndEnumTypes(FInterface fInterface, Boolean includingTransitiveTypes) {
		getAllComplexAndEnumTypes(fInterface, includingTransitiveTypes, true, true, true, true)
	}

	def getAllComplexAndEnumTypes(FInterface fInterface, Boolean includingTransitiveTypes, boolean methods, boolean readAttributes, boolean writeAttributes, boolean broadcasts){
		val typeList = new HashSet<Object>();
		if (methods){
			for (method : fInterface.methods) {
				for(returnParameter : getOutputParameters(method)){
				if (returnParameter != null && (isComplex(returnParameter.type) || isEnum(returnParameter.type))) {
					typeList.add(getDatatype(returnParameter.type));
				}

				}
				for (inputParameter : getInputParameters(method)) {
					if (inputParameter != null && (isComplex(inputParameter.type) || isEnum(inputParameter.type))) {
						typeList.add(getDatatype(inputParameter.type));
					}
				}
			}
		}

		for (attribute : getAttributes(fInterface)) {
			if ((readAttributes && attribute.readable) || writeAttributes && attribute.writable){
				if (isComplex(attribute.type) || isEnum(attribute.type)) {
					typeList.add(getDatatype(attribute.type));
				}
			}
		}

		if (broadcasts) {
			for (broadcast : fInterface.broadcasts) {
				for (outParameter : getOutputParameters(broadcast)) {
					if (outParameter != null && (isComplex(outParameter.type) || isEnum(outParameter.type))) {
						typeList.add(getDatatype(outParameter.type));
					}
				}
			}
		}

		if (includingTransitiveTypes){
			var returnValue = new HashSet<Object>()
			getAllReferredDatatypes(typeList, returnValue)
			return returnValue
		}
		else{
			return typeList;
		}
	}

	def private getAllReferredDatatypes(HashSet<Object> list, HashSet<Object> cache) {
		for(element : list){
			if (!cache.contains(element)){
				cache.add(element)
				if (element instanceof FCompoundType){

				}
			}
		}
	}

	def getAllComplexAndEnumTypes(FInterface fInterface) {
		getAllComplexAndEnumTypes(fInterface, false)
	}

	def getAllComplexAndEnumTypes(FInterface fInterface, boolean methods, boolean readAttributes, boolean writeAttributes, boolean broadcasts) {
		getAllComplexAndEnumTypes(fInterface, false, methods, readAttributes, writeAttributes, broadcasts)
	}

	def getDataTypes(FModel fModel) {
		val referencedFTypes = new HashSet<FType>()

		fModel.typeCollections.forEach[referencedFTypes.addAll(types)]

		fModel.interfaces.forEach[referencedFTypes.addAll(types)]

		return referencedFTypes
	}

	def getComplexDataTypes(FModel fModel) {
		getDataTypes(fModel).filter(type | type.isComplex)
	}

	def getPrimitiveDataTypes() {
		FBasicTypeId::values.filter[value != FBasicTypeId::UNDEFINED_VALUE && value != FBasicTypeId::BYTE_BUFFER_VALUE] // filter out "undefined" and "buffer" data type
	}

	def getEnumDataTypes(FModel fModel) {
		getDataTypes(fModel).filter(type | type.isEnum)
	}

	def getEnumElements(FEnumerationType enumType) {
		enumType.enumerators
	}

	def Iterable<FField> getMembersRecursive(FType datatype) {
		val datatypeInternal = getDatatype(datatype)
		val result = new ArrayList<FField>
		if (datatypeInternal instanceof FCompoundType) {
			val compoundType = datatype as FCompoundType
			result.addAll(compoundType.elements)
		}

		if (datatypeInternal instanceof FStructType && (datatypeInternal as FStructType).base!=null) {
			return Iterables::concat(getMembersRecursive((datatypeInternal as FStructType).base), result);
		} else if (datatypeInternal instanceof FUnionType && (datatypeInternal as FUnionType).base!=null) {
			return Iterables::concat(getMembersRecursive((datatypeInternal as FUnionType).base), result);
		}
		else{
			return result;
		}
	}

	def getMembers(FCompoundType compoundType) {
		compoundType.elements
	}

	def getEnumMembers(FCompoundType datatype) {
		datatype.elements.filter(element | isEnum(element.type));
	}

	def getComplexMembers(FCompoundType datatype) {
		datatype.elements.filter(element | isComplex(element.type));
	}

	def getComplexAndEnumMembers(FCompoundType datatype) {
		datatype.elements.filter(element | isComplex(element.type) || isEnum(element.type) || isArray(element));
	}

	def FCompoundType getComplexType(FType type){
		if (type == null){
			return null;
		}
		if (type instanceof FArrayType){
			return getComplexType((type as FArrayType).elementType)
		}
		else if (type instanceof FCompoundType){
			return type as FCompoundType;
		}
		else if (type instanceof FTypeDef){
			return getComplexType((type as FTypeDef).actualType)
		}
	}

	def FCompoundType getComplexType(FTypeRef type){
		if (type==null){
			return null;
		}
		else{
			return getComplexType(type.derived)
		}
	}

	def FEnumerationType getEnumType(FTypeRef type){
		if (type==null){
			return null;
		}
		else{
			return getEnumType(type.derived)
		}
	}

	def FEnumerationType getEnumType(FType type){
		if (type == null){
			return null;
		}
		if (type instanceof FArrayType){
			return getEnumType((type as FArrayType).elementType)
		}
		else if (type instanceof FEnumerationType){
			return type as FEnumerationType;
		}
		else if (type instanceof FTypeDef){
			return getEnumType((type as FTypeDef).actualType)
		}
	}

	def boolean isComplex(FType type) {
		if (type==null){
			return false
		}
		if (type instanceof FArrayType){
			return isComplex((type as FArrayType).elementType)
		}
		if (type instanceof FCompoundType){
			return true
		}
		if (type instanceof FTypeDef){
			return isComplex((type as FTypeDef).actualType)
		}
		if (type instanceof FEnumerationType){
			return false
		}
		if (type instanceof FMapType){
			throw new IllegalStateException("JoynGeneratorExtensions.xtend: isComplex for map types is not implemented!")
//			val mapType = type as FMapType
//			return (isComplex(mapType.keyType) || isComplex(mapType.valueType));
		}
		return false
	}

	def boolean isComplex(FTypeRef typeRef) {
		if (typeRef == null){
			return false;
		}
		if (typeRef.derived!=null){
			return isComplex(typeRef.derived)
		}
		return false
	}

	def boolean isEnum(FType type) {
		if (type==null){
			return false
		}
		if (type instanceof FArrayType){
			isEnum((type as FArrayType).elementType)
		}
		if (type instanceof FStructType || type instanceof FUnionType){
			return false
		}
		if (type instanceof FTypeDef){
			isEnum((type as FTypeDef).actualType)
		}
		if (type instanceof FEnumerationType){
			return true
		}
		if (type instanceof FMapType){
			throw new IllegalStateException("JoynGeneratorExtensions.xtend: isEnum for map types is not implemented!")
//			val mapType = type as FMapType
//			return (isComplex(mapType.keyType) || isComplex(mapType.valueType));
		}
		return false
	}

	def boolean isEnum(FTypeRef typeRef) {
		if (typeRef == null){
			return false;
		}
		if (typeRef.derived!=null){
			return isEnum(typeRef.derived)
		}
		return false
	}

	def boolean isPrimitive(FTypeRef typeRef){
		if (typeRef== null){
			return false;
		} 

		if (typeRef.predefined!=null && typeRef.predefined!=FBasicTypeId::UNDEFINED){
			return true;
		}
		else {
			return isPrimitive(typeRef.derived)
		}
	}

	def boolean isPrimitive(FType type){
		if (type instanceof FMapType){
			throw new IllegalStateException("JoynGeneratorExtensions.xtend: isPrimitive for map types is not implemented!")
		}
		else {
			if (type instanceof FArrayType){
				return isPrimitive((type as FArrayType).elementType)
			}
			if (type instanceof FTypeDef){
				return isPrimitive((type as FTypeDef).actualType)
			}
		}
		return false;
	}

	def FBasicTypeId getPrimitive(FType type){
		if (type == null){
			return null;
		}
		if (type instanceof FArrayType){
			return getPrimitive((type as FArrayType).elementType)
		}
		if (type instanceof FTypeDef){
			return getPrimitive((type as FTypeDef).actualType)
		}
	}

	def FBasicTypeId getPrimitive(FTypeRef type){
		if (type.predefined != null && type.predefined!=FBasicTypeId::UNDEFINED){
			return type.predefined
		}
		else{
			return getPrimitive(type.derived)
		}
	}

	def String typeName(Object type) {
		if (type instanceof FType){
			(type as FType).joynrName
		}
		else if (type instanceof FBasicTypeId){
			(type as FBasicTypeId).joynrName
		}
		else{
			return null;
			// throw new IllegalStateException("The typename cannot be resolved" + (if (type == null) ", because the invoked parameter is null " else (" for type " + type)))
		}
	}

	def getAllTypes(FInterface fInterface) {
		val typeList = new HashMap<String, Object>()
		for (method : getMethods(fInterface)) {
			for (returnParameter : getOutputParameters(method)) {
				if (returnParameter != null) {
					val datatype = getDatatype(returnParameter.type)
					val typename = datatype.typeName;
					if (typename != null){
						if (!typeList.containsKey(typename)){
							typeList.put(datatype.typeName, datatype);
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
					val typename = datatype.typeName;
					if (typename != null){
						if (!typeList.containsKey(typename)){
							typeList.put(datatype.typeName, datatype);
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
			if (!typeList.containsKey(datatype.typeName)){
				typeList.put(datatype.typeName, datatype);
			}
		}
		return typeList.values;
	}

	def String getDefaultValue(FTypedElement element)

	def prependCommaIfNotEmpty(String input) {
		if (input.equals("")) {
			return input;
		}
		return ", " + input;
	}

	def getCommaSeperatedUntypedParameterList(FMethod method) {
		val returnStringBuilder = new StringBuilder();
		for (param : getInputParameters(method)) {
			returnStringBuilder.append(param.joynrName)
			returnStringBuilder.append(", ")
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		}
		return returnString.substring(0, returnString.length() - 2); //remove the last ,
	}

	def String getOneLineWarning()

	def addWarningsToEachLine(String input) {
		val str = new StringBuilder();

		val reader = new BufferedReader(new StringReader(input));
		try {
			var line = "";
			while ((line = reader.readLine()) != null) {
				val outputLine = getOneLineWarning() + line + "\n";
				str.append(outputLine);
			}

		} catch (IOException e) {
			e.printStackTrace();
		}
		return str.toString();
	}

	def isReadable(FAttribute field) {
		return true
	}

	def boolean isWritable(FAttribute field) {
		return !field.isReadonly
	}

	def isNotifiable(FAttribute field) {
		return field.observable
	}

	def boolean isReadonly(FAttribute field)

	def boolean isObservable(FAttribute field)

	def boolean needsListImport (FInterface serviceInterface) {
		needsListImport(serviceInterface, true, true);
	}

	def boolean needsListImport(FInterface fInterface, boolean includeMethods, boolean includeAttributes) {
		if (includeMethods){
			//need to Import the dependencies to Lists if either input or output parameters of one of the methods are a list.
			for (method : getMethods(fInterface)) {
				for (output : getOutputParameters(method)) {
					if (output != null && isArray(output)) return true;
				}

				for (input : getInputParameters(method)) {
					if (input != null && isArray(input)) return true;
				}
			}
		}

		if (includeAttributes){
					//need to Import the dependencies to Lists if one of the attributes is a list.
			for (attribute : getAttributes(fInterface)) {
				if (attribute != null && isArray(attribute)) {
					return true;
				}
			}
		}
		return false;
	}

	/*
	 * Returns true if the member is a 1 Dimensional list, returns false if not. (2D Lists are not supported)
	 */
	def boolean isArray(FTypedElement typedElement) {
		return typedElement.array== '[]'
	}

	def boolean hasExtendsDeclaration(FCompoundType datatype) {
		if (datatype instanceof FStructType && (datatype as FStructType).base!=null) {
			return true
		} else if (datatype instanceof FUnionType && (datatype as FUnionType).base!=null) {
			return true
		}
		return false
	}

	def FCompoundType getExtendedType(FCompoundType datatype) {
		if (datatype instanceof FStructType && (datatype as FStructType).base!=null) {
			return (datatype as FStructType).base
		} else if (datatype instanceof FUnionType && (datatype as FUnionType).base!=null) {
			return (datatype as FUnionType).base
		}
		return null
	}

	def getDatatype(FType type){
		if (type instanceof FArrayType){
			return getDatatype((type as FArrayType).elementType)
		}
		if (type instanceof FCompoundType){
			return type
		}
		if (type instanceof FTypeDef){
			return getDatatype((type as FTypeDef).actualType)
		}
		if (type instanceof FEnumerationType){
			return type
		}
		if (type instanceof FMapType){
			throw new IllegalStateException("JoynGeneratorExtensions.xtend: isComplex for map types is not implemented!")
//			val mapType = type as FMapType
//			return (isComplex(mapType.keyType) || isComplex(mapType.valueType));
		}

	}

	def Object getDatatype(FTypeRef typeRef) {
		if (typeRef == null){
			return null
		}

		if (typeRef.derived!=null){
			return getDatatype(typeRef.derived)
		}
		return typeRef.predefined
	}

	def boolean isLong(FBasicTypeId type) {
		return type == FBasicTypeId::INT64;
	}

	def boolean isDouble(FBasicTypeId type) {
		return type == FBasicTypeId::DOUBLE;
	}

	def boolean isFloat(FBasicTypeId type) {
		return type == FBasicTypeId::FLOAT;
	}

	def boolean isBool(FBasicTypeId type) {
		return type == FBasicTypeId::BOOLEAN;
	}

	def boolean isInt(FBasicTypeId type) {
		return type == FBasicTypeId::INT16 || type == FBasicTypeId::INT32;
	}

	def boolean isString(FBasicTypeId type) {
		return type == FBasicTypeId::STRING;
	}

	def boolean isByte(FBasicTypeId type) {
		return type == FBasicTypeId::UINT8;
	}

	def boolean isByteBuffer(FBasicTypeId type) {
		return type == FBasicTypeId::BYTE_BUFFER;
	}

	def boolean isByteBuffer(FTypeRef typeRef) {
		if (typeRef == null){
			return false;
		}
		return isByteBuffer(getPrimitive(typeRef))
	}

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

	def escapeQuotes(String string) {
		string.replace('\"', '\\\"')
	}

	def getJoynrGenerationPrefix() {
		"joynr"
	}

	def joynrName(FTypedElement element){
		element.name
	}

	def joynrName(FBasicTypeId type){
		type.getName
	}

	def joynrName(FModel model){
		model.name
	}

	def joynrName(FModelElement element){
		element.name
	}

	def joynrName(FEnumerator enumElement){
		enumElement.name.toUpperCase
	}

	def joynrName(FType type){
		type.name
	}

	def joynrName(FField member) {
		member.name
	}

	def joynrName(FInterface iFace){
		iFace.name
	}

	def joynrName(FMethod method) {
		method.name
	}

	def joynrName(FAttribute attribute) {
		attribute.name
	}

	def joynrName(FBroadcast event) {
		event.name
	}

	def joynrName(FArgument argument){
		argument.name
	}

	def isSelective(FBroadcast broadcast) {
		return broadcast.selective != null
	}
}
