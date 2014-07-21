package io.joynr.generator.util
/*
 * !!!
 *
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import java.util.Collections
import java.util.HashMap
import java.util.Iterator
import java.util.Map
import java.util.TreeSet
import org.franca.core.franca.FArgument
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypedElement

class JoynrJavaGeneratorExtensions extends JoynrGeneratorExtensions {
	
//    @Inject private extension FrancaGeneratorExtensions

	private Map<FBasicTypeId,String> primitiveDataTypeDefaultMap;
//	private Map<FBasicTypeId,String> primitiveDataTypeNameMap;

	def String getNamespaceStarter(FInterface interfaceType) {
		getNamespaceStarter(getPackageNames(interfaceType));
	}

	def String getNamespaceStarter(FType datatype) {
		getNamespaceStarter(getPackageNames(datatype));
	}

	def String getNamespaceEnder(FInterface interfaceType) {
		getNamespaceEnder(getPackageNames(interfaceType));
	}
	
	def String getNamespaceEnder(FType datatype) {
		getNamespaceEnder(getPackageNames(datatype));
	}

	def private String getNamespaceStarter(Iterator<String> packageList){
		return getNamespaceStarterFromPackageList(packageList);
	}

	def String getNamespaceStarterFromPackageList(Iterator<String> packageList){
		var sb = new StringBuilder();
		while(packageList.hasNext){
			sb.append("namespace " + packageList.next + "{ " );
		}
		return sb.toString();
	}

	def private String getNamespaceEnder(Iterator<String> packageList){
		return getNameSpaceEnderFromPackageList(packageList);
	}

	def String getNameSpaceEnderFromPackageList(Iterator<String> packageList){
		var sb = new StringBuilder();
		while(packageList.hasNext){
			sb.insert(0, "} /* namespace " + packageList.next + " */ " );
		}
		return sb.toString();
	}

	new () {
/*
		val Map<FBasicTypeId,String> aMap = new HashMap<FBasicTypeId,String>();
		aMap.put(FBasicTypeId::BOOLEAN, "bool");
		aMap.put(FBasicTypeId::STRING, "QString");
		aMap.put(FBasicTypeId::DOUBLE,"double");
		aMap.put(FBasicTypeId::INT16,"int");
		aMap.put(FBasicTypeId::INT32,"int");
		aMap.put(FBasicTypeId::INT64,"qint64");
		aMap.put(FBasicTypeId::INT8,"qint8");
		aMap.put(FBasicTypeId::UNDEFINED,"void");
		primitiveDataTypeNameMap = Collections::unmodifiableMap(aMap);
*/

	val Map<FBasicTypeId,String> bMap = new HashMap<FBasicTypeId,String>();		
		bMap.put(FBasicTypeId::BOOLEAN, "false");
		bMap.put(FBasicTypeId::INT8, "0");
		bMap.put(FBasicTypeId::UINT8, "0");
		bMap.put(FBasicTypeId::INT16, "0");
		bMap.put(FBasicTypeId::UINT16, "0");
		bMap.put(FBasicTypeId::INT32, "0");
		bMap.put(FBasicTypeId::UINT32, "0");
		bMap.put(FBasicTypeId::INT64, "0L");
		bMap.put(FBasicTypeId::UINT64, "0l");
		//see bug JOYN-1521: floats are interpreted as double
		bMap.put(FBasicTypeId::FLOAT, "0d");
		bMap.put(FBasicTypeId::DOUBLE, "0d");
		bMap.put(FBasicTypeId::STRING, "\"\"");
		bMap.put(FBasicTypeId::BYTE_BUFFER, "new byte[0]");
		bMap.put(FBasicTypeId::UNDEFINED,"");
		
		primitiveDataTypeDefaultMap = Collections::unmodifiableMap(bMap);
	}
	 
	def getCommaSeperatedTypedOutputParameterList(FMethod method) {
		val returnStringBuilder = new StringBuilder();
		for(FArgument argument : getOutputParameters(method)){
			returnStringBuilder.append(getMappedDatatypeOrList(argument));
			returnStringBuilder.append("& ");
			returnStringBuilder.append(argument.joynrName);
			returnStringBuilder.append(", ");
		}
        val returnString = returnStringBuilder.toString();
        if (returnString.length() == 0) {
            return "";
        }
        else{
	        return returnString.substring(0, returnString.length() - 2); //remove the last ,
        }
	}
	
	def getCommaSeperatedUntypedOutputParameterList(FMethod method) {
		val returnStringBuilder = new StringBuilder();
		for(FArgument argument : getOutputParameters(method)){
			returnStringBuilder.append(argument.joynrName);
			returnStringBuilder.append(", ");
		}
        val returnString = returnStringBuilder.toString();
        if (returnString.length() == 0) {
            return "";
        }
        else{
	        return returnString.substring(0, returnString.length() - 2); //remove the last ,
        }
	}
	 
	def getCommaSeperatedTypedParameterList(FMethod method) {
        val returnStringBuilder = new StringBuilder();
        for (param : getInputParameters(method)) {
        	returnStringBuilder.append(getMappedDatatypeOrList(param));
        	returnStringBuilder.append(" ");
        	returnStringBuilder.append(param.joynrName);
        	returnStringBuilder.append(", ");
        }
        val returnString = returnStringBuilder.toString();
        if (returnString.length() == 0) {
            return "";
        }
        else{
	        return returnString.substring(0, returnString.length() - 2); //remove the last ,
        }
    }
	override getMappedDatatype(FType datatype) {
		return datatype.typeName
	}
		
	override getMappedDatatypeOrList(FType datatype, boolean array) {
		val mappedDatatype = getMappedDatatype(datatype);
		if (array) {
			return "List<" + getObjectDataTypeForPlainType(mappedDatatype) + ">";
		} else {
			return mappedDatatype;
		}
	}

	override getMappedDatatypeOrList(FBasicTypeId datatype, boolean array) {
		val mappedDatatype = getPrimitiveTypeName(datatype);
		if (array) {
			return "List<" + getObjectDataTypeForPlainType(mappedDatatype) + ">";
		} else {
			return mappedDatatype;
		}
	}
	
	override getDefaultValue(FTypedElement element) {
		//default values are not supported (currently) by the Franca IDL 
		if (1==0){
//		if (member.getDEFAULTVALUE()!=null && !member.getDEFAULTVALUE().isEmpty()){
//			if (isEnum(member)){
//				val ENUMDATATYPETYPE enumDatatype = getDatatype(id) as ENUMDATATYPETYPE
//				for (ENUMELEMENTTYPE element : getEnumElements(enumDatatype)){
//					if (element.VALUE == member.DEFAULTVALUE){
//						return enumDatatype.SHORTNAME.toFirstUpper + "::" + element.SYNONYM
//					}
//				}
//				return getPackagePath(enumDatatype, "::") + "::" + enumDatatype.SHORTNAME.toFirstUpper + "::" +  (enumDatatype.ENUMERATIONELEMENTS.ENUMELEMENT.get(0) as ENUMELEMENTTYPE).SYNONYM
//			}
////			else if (isLong(member.getDATATYPEREF().getIDREF())){
////				return member.getDEFAULTVALUE() + "L"
////			}
////			else if (isDouble(member.getDATATYPEREF().getIDREF())){
////				return member.getDEFAULTVALUE() + "d"
////			}
//			else{
//				return member.getDEFAULTVALUE();
//			}
		} else if (isComplex(element.type)) {
			if ((isArray(element))){
				return "new ArrayList<" + element.type.complexType.joynrName + ">()";
			}
			else{
				return "new " + element.type.complexType.joynrName + "()";
			} 
		} else if (isEnum(element.type)){
			if ((isArray(element))){
				return "new ArrayList<" + element.type.enumType.joynrName + ">()";
			}
			else{
				return  element.type.enumType.joynrName + "." + element.type.enumType.enumerators.get(0).joynrName;
			} 
		} else if (!primitiveDataTypeDefaultMap.containsKey(element.type.predefined)) {
 			return "NaN";
 		} else if (isPrimitive(element.type)) {
			if ((isArray(element))){
				return "new ArrayList<" + getPrimitiveTypeName(getPrimitive(element.type)) + ">()";
			}
			else{
				return primitiveDataTypeDefaultMap.get(element.type.predefined);
			} 
		}
		
	}

	
	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype){
		getRequiredIncludesFor(datatype, true);
	}
	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype, boolean includingExendedType){
		val members = getComplexAndEnumMembers(datatype);
		
		val typeList = new TreeSet<String>();
		if (hasExtendsDeclaration(datatype)){
			if (includingExendedType){
				typeList.add(getIncludeOf(getExtendedType(datatype)))
			}

			typeList.addAll(getRequiredIncludesFor(getExtendedType(datatype), false))

		}

		for (member : members) {
			val type = getDatatype(member.type);
			if (type instanceof FType){
				typeList.add(getIncludeOf(type as FType));
			}
		}	
		return typeList;
	}
		
	def Iterable<String> getRequiredIncludesFor(FInterface serviceInterface){
		val includeSet = new TreeSet<String>();
		for(datatype: getAllComplexAndEnumTypes(serviceInterface)){
			if (datatype instanceof FType){
				includeSet.add(getIncludeOf(datatype as FType));
			}
//			else{
//				includeSet.add(getIncludeOf(datatype as FBasicTypeId));
//			}
		}
		return includeSet;
	}
	
	def String getIncludeOf(FType dataType) {
		return getPackagePathWithJoynrPrefix(dataType, ".") + "." + dataType.joynrName;
	}
	
	override String getOneLineWarning() {
		//return ""
		return "/* Generated Code */  "
	}
	
	def String getTypedParameterListJavaRpc(FMethod method){
		var sb = new StringBuilder();
		val params = getInputParameters(method)
		for (param : params) {
			// is it a list type?
			if(! getMappedDatatypeOrList(param).contains("List")){
				sb.append("@JoynrRpcParam")
				sb.append("(\"" + param.joynrName + "\")")
				sb.append(" "+ getMappedDatatypeOrList(param))
				sb.append(" "+ param.joynrName)
				sb.append(", ")
			}else { //TODO clean this up, move to javaGeneratorUtil.xtend!
				sb.append("@JoynrRpcParam") 
				sb.append("(value=\"" + param.joynrName 
						+ "\", deserialisationType=List"
						+ getMappedDatatypeOrList(param).substring(5, getMappedDatatypeOrList(param).length()-1) 
						+ "Token.class)")
				sb.append(" "+ getMappedDatatypeOrList(param))
				sb.append(" "+ param.joynrName)
				sb.append(", ")				
			}
		}
		if (sb.length()==0){
			return ""; 
		}
		else{
			return sb.toString.substring(0,  sb.length() - 2); //remove the last ,
		}
	}

	def String getTypedParameterListJavaTypeReference(FMethod method){
		val sb = new StringBuilder()
		val params = getInputParameters(method)
		for (param : params) {
			sb.append("public static class "+getMappedDatatypeOrList(param)+ "Token extends TypeReference<"+getMappedDatatypeOrList(param)+" > {}\n")
		}
		sb.append("public static class "+ getMappedOutputParameter(method)+ "Token extends TypeReference<"+getMappedOutputParameter(method)+" > {}\n")
		if (sb.length()==0){
			return ""
		}
		return sb.toString
	}
	
    override boolean needsListImport (FInterface serviceInterface) {
    	//only need to Import the dependencies to Lists if either input or output parameters of one of the methods are a list.
        for (method : getMethods(serviceInterface)) {
       		if (method!=null && getOutputParameters(method)!=null &&  getOutputParameters(method).iterator.hasNext && isArray(getOutputParameters(method).iterator.next)  ) return true;
       		
        	for (input : getInputParameters(method)){
        		if (input!=null && isArray(input)) return true;
        	}
        }
        
        for (attribute : getAttributes(serviceInterface)) {
        	if (attribute!=null && isArray(attribute)) {
        		return true;
        	}
        }
        return false;
    }

    override isReadonly(FAttribute fAttribute) { fAttribute.readonly }

    override isObservable(FAttribute fAttribute) { !fAttribute.noSubscriptions }

	override getPrimitiveTypeName(FBasicTypeId basicType) {
		switch basicType {
			case FBasicTypeId::BOOLEAN: "Boolean"
			case FBasicTypeId::INT8: "Byte"
			case FBasicTypeId::UINT8: "Byte"
			case FBasicTypeId::INT16: "Integer"
			case FBasicTypeId::UINT16: "Integer"
			case FBasicTypeId::INT32: "Integer"
			case FBasicTypeId::UINT32: "Integer"
			case FBasicTypeId::INT64: "Long"
			case FBasicTypeId::UINT64: "Long"
			case FBasicTypeId::FLOAT: "Double"
			case FBasicTypeId::DOUBLE: "Double"
			case FBasicTypeId::STRING: "String"
			case FBasicTypeId::BYTE_BUFFER: "byte[]"
			default: throw new IllegalArgumentException("Unsupported basic type: " + basicType.joynrName)
        }
		// francaExtensions.getPrimitiveTypeName(basicType)
	}
	
	def String getObjectDataTypeForPlainType(String plainType) {
		
		var type = plainType.toLowerCase
		switch (plainType) {
			case FBasicTypeId::BOOLEAN.name: type = "Boolean"
			case FBasicTypeId::INT8.name: type = "Byte"
			case FBasicTypeId::UINT8.name: type = "Byte"
			case FBasicTypeId::INT16.name: type = "Integer"
			case FBasicTypeId::UINT16.name: type = "Integer"
			case FBasicTypeId::INT32.name: type = "Integer"
			case FBasicTypeId::UINT32.name: type = "Integer"
			case FBasicTypeId::INT64.name: type = "Long"
			case FBasicTypeId::UINT64.name: type = "Long"
			case FBasicTypeId::FLOAT.name: type = "Double"
			case FBasicTypeId::DOUBLE.name: type = "Double"
			case FBasicTypeId::STRING.name: type = "String"
			case FBasicTypeId::BYTE_BUFFER.name: type = "byte[]"
			case "void": type = "Void"
			default :  type = plainType

		}

		return type
	}
	
	def String getTokenTypeForArrayType(String plainType) {
			
		if(plainType.contains("List<")) {
			return "List" + getObjectDataTypeForPlainType(plainType.substring(5, plainType.length-1));
		}
		else{
			return getObjectDataTypeForPlainType(plainType);
		}
	}
	
		// Returns true if a class or superclass has array members	
	def boolean hasArrayMembers(FCompoundType datatype){
		for (member : datatype.members) {
			if (isArray(member)){
				return true
			}
		}
		// Check any super classes 
		if (hasExtendsDeclaration(datatype)) {
			return hasArrayMembers(datatype.extendedType)
		}
		return false
	}

	// Returns true if a class has to create lists in its constructor	
	def boolean hasListsInConstructor(FCompoundType datatype){
		for (member : datatype.members) {
			if (isArray(member)){
				return true
			}
		}
		return false
	}	

    def String getJoynFullyQualifiedTypeName(FTypedElement typedElement){
    	if (typedElement.array == '[]'){
    		return "List"
    	}
    	if (typedElement.type.derived != null){
    		getJoynFullyQualifiedTypeName(typedElement.type.derived)
    	}
    	else{
    		getPrimitiveTypeName(typedElement.type.predefined)
    	}
    }

	def getJoynFullyQualifiedTypeName(FType type) {
		joynTypePackagePrefix + "." + type.mappedDatatype
	}
	
	def getJoynTypePackagePrefix(){
		joynrGenerationPrefix
	}
}