package io.joynr.generator.util
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

import com.google.common.collect.Iterables
import com.google.common.collect.Iterators
import java.io.BufferedReader
import java.io.IOException
import java.io.StringReader
import java.util.ArrayList
import java.util.HashMap
import java.util.HashSet
import org.eclipse.emf.ecore.impl.BasicEObjectImpl
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FArgument
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FEnumerator
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
import com.google.inject.Inject
import com.google.inject.name.Named
import org.eclipse.emf.common.util.EList
import org.eclipse.emf.common.util.BasicEList
import org.eclipse.emf.ecore.util.EcoreUtil
import org.franca.core.franca.FTypeCollection

abstract class JoynrGeneratorExtensions {

	public final static String JOYNR_GENERATOR_GENERATE = "JOYNR_GENERATOR_GENERATE";
	public final static String JOYNR_GENERATOR_CLEAN = "JOYNR_GENERATOR_CLEAN";

	@Inject(optional = true)
	@Named(JOYNR_GENERATOR_GENERATE)
	public boolean generate = true;

	@Inject(optional = true)
	@Named(JOYNR_GENERATOR_CLEAN)
	public boolean clean = false;

	def Iterable<FInterface> getInterfaces(FModel model) {
		return model.interfaces
	}

	def Iterable<FArgument> getOutputParameters(FMethod method) {
		if (method == null || method.outArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			return method.outArgs.filterNull
		}
	}

	def Iterable<FArgument> getOutputParameters(FBroadcast event) {
		if (event == null || event.outArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			return event.outArgs.filterNull
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

	def boolean isPartOfTypeCollection(FType datatype) {
		return datatype.eContainer instanceof FTypeCollection;
	}
	def String getTypeCollectionName(FType datatype) {
		if(!datatype.isPartOfTypeCollection) {
			throw new IllegalStateException(
					"Datatype " + datatype.joynrName + " is not part of a type collection."
					+ " Please call isPartOfTypeCollection before calling this method."
			);
		}
		return (datatype.eContainer as FTypeCollection).joynrName;
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
		getAllComplexAndEnumTypes(fInterface, includingTransitiveTypes, true, true, true, true, true)
	}

	def getAllComplexAndEnumTypes(
			FInterface fInterface,
			Boolean includingTransitiveTypes,
			boolean methods,
			boolean readAttributes,
			boolean writeAttributes,
			boolean notifyAttributes,
			boolean broadcasts
	) {
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
			if ((readAttributes && attribute.readable)
					|| (writeAttributes && attribute.writable)
					|| (notifyAttributes && attribute.notifiable)
			) {
				if (isComplex(attribute.type) || isEnum(attribute.type)) {
					typeList.add(getDatatype(attribute.type));
				}
			}
		}

		if (broadcasts) {
			for (broadcast : fInterface.broadcasts) {
				typeList.addAll(getAllComplexAndEnumTypes(broadcast));
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

	def getAllComplexAndEnumTypes(FBroadcast broadcast) {
		val typeList = new HashSet<Object>();
		for (outParameter : getOutputParameters(broadcast)) {
			if (outParameter != null && (isComplex(outParameter.type) || isEnum(outParameter.type))) {
				typeList.add(getDatatype(outParameter.type));
			}
		}
		return typeList;
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

	def getAllComplexAndEnumTypes(
			FInterface fInterface,
			boolean methods,
			boolean readAttributes,
			boolean writeAttributes,
			boolean notifyAttributes,
			boolean broadcasts
	) {
		getAllComplexAndEnumTypes(fInterface, false, methods, readAttributes, writeAttributes, notifyAttributes, broadcasts)
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

	def EList<FEnumerator> getEnumElementsAndBaseEnumElements(FEnumerationType enumType) {
		if (hasExtendsDeclaration(enumType)) {
			val baseEnumType = getExtendedType(enumType)
			var enumAndBaseEnumElements = new BasicEList<FEnumerator>()
			val baseEnumElements = getEnumElementsAndBaseEnumElements(baseEnumType)
			enumAndBaseEnumElements.addAll(EcoreUtil.copyAll(baseEnumElements))
			val enumElements = getEnumElements(enumType)
			enumAndBaseEnumElements.addAll(EcoreUtil.copyAll(enumElements))
			return enumAndBaseEnumElements
		} else {
			return getEnumElements(enumType)
		}
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

	def String joynrName(Object type) {
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

	def prependCommaIfNotEmpty(String input) {
		if (input.equals("")) {
			return input;
		}
		return ", " + input;
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

	def boolean isReadonly(FAttribute fAttribute) {
		fAttribute.readonly
	}

	def boolean isObservable(FAttribute fAttribute) {
		!fAttribute.noSubscriptions
	}

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

	def boolean hasExtendsDeclaration(FEnumerationType datatype) {
		if (datatype.base!=null) {
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

	def FEnumerationType getExtendedType(FEnumerationType datatype) {
		if (datatype.base!=null) {
			return datatype.base
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

	/**
	 * @return a mapping from method names to the number of their overloads.
	 */
	def overloadedMethodCounts(Iterable<FMethod> methods) {
		var methodNameToCount = new HashMap<String, Integer>();

		for (FMethod method : methods) {
			var Integer count = 1;
			if (methodNameToCount.containsKey(method.name)) {
				count = methodNameToCount.get(method.name);
				count++;
			}
			methodNameToCount.put(method.name, count);
		}
		return methodNameToCount;
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		InterfaceTemplate generator,
		FInterface serviceInterface
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(serviceInterface).toString);
		}
	}

	def generateFile(IFileSystemAccess fsa,
		String path,
		BroadcastTemplate generator,
		FInterface serviceInterface,
		FBroadcast broadcast
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(serviceInterface, broadcast).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		EnumTemplate generator,
		FEnumerationType enumType
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(enumType).toString);
		}
	}

	def generateFile(
		IFileSystemAccess fsa,
		String path,
		CompoundTypeTemplate generator,
		FCompoundType compoundType
	) {
		if (clean) {
			fsa.deleteFile(path);
		}
		if (generate) {
			fsa.generateFile(path, generator.generate(compoundType).toString);
		}
	}

}
