package io.joynr.generator.cpp.util
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

import java.util.Collections

import java.util.HashMap
import java.util.Iterator
import java.util.Map
import java.util.TreeSet
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FArgument
import com.google.inject.Inject
import com.google.inject.name.Namedimport org.franca.core.franca.FModelElement
import java.io.File

class JoynrCppGeneratorExtensions extends CommonApiJoynrGeneratorExtensions {
	
	@Inject @Named("generationId")
	String dllExportName;

	private Map<FBasicTypeId,String> primitiveDataTypeDefaultMap;

	// Convert ByteBuffers into QByteArrays
	override getPrimitiveTypeName(FBasicTypeId basicType) {
		if (basicType == FBasicTypeId::BYTE_BUFFER) {
			return "QByteArray"
		}
		super.getPrimitiveTypeName(basicType)
	}

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
		sb.append("namespace " + joynrGenerationPrefix + " { ");
		while(packageList.hasNext){
			sb.append("namespace " + packageList.next + " { " );
		}
		return sb.toString();
	}

	def private String getNamespaceEnder(Iterator<String> packageList){
		return getNameSpaceEnderFromPackageList(packageList);
	}

	def String getNameSpaceEnderFromPackageList(Iterator<String> packageList){
		var sb = new StringBuilder();
		sb.append("} /* namespace " + joynrGenerationPrefix + " */ ");
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
		bMap.put(FBasicTypeId::INT8, "-1");
		bMap.put(FBasicTypeId::UINT8, "-1");
		bMap.put(FBasicTypeId::INT16, "-1");
		bMap.put(FBasicTypeId::UINT16, "-1");
		bMap.put(FBasicTypeId::INT32, "-1");
		bMap.put(FBasicTypeId::UINT32, "-1");
		bMap.put(FBasicTypeId::INT64, "-1");
		bMap.put(FBasicTypeId::UINT64, "-1");
		bMap.put(FBasicTypeId::FLOAT, "-1");
		bMap.put(FBasicTypeId::DOUBLE, "-1");
		bMap.put(FBasicTypeId::STRING, "\"\"");
		bMap.put(FBasicTypeId::BYTE_BUFFER, "\"\"");
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

	def buildPackagePath(FType datatype, String separator) {
		if (datatype == null) {
			return "";
		}
		var packagepath = "";
		try {
			packagepath = getPackagePathWithJoynrPrefix(datatype, separator);
		} catch (IllegalStateException e){
			//	if an illegal StateException has been thrown, we tried to get the package for a primitive type, so the packagepath stays empty.
		} 
		if (packagepath!="") { 
			packagepath = packagepath + separator;
		};
		return packagepath;
	}
         
	override getMappedDatatype(FType datatype) {
		val packagepath = buildPackagePath(datatype, "::");
		if (isEnum(datatype)){
			return  packagepath + datatype.joynrName+ "::" + getNestedEnumName();
		}
		else{
			return  packagepath + datatype.joynrName  //if we don't know the type, we have to assume its a complex datatype defined somewhere else.
		}
	}

	override getMappedDatatypeOrList(FType datatype, boolean array) {
		val mappedDatatype = getMappedDatatype(datatype);
		if (array) {
			return "QList<" + mappedDatatype + "> ";
		} else {
			return mappedDatatype;
		}
	}

	override getMappedDatatypeOrList(FBasicTypeId datatype, boolean array) {
		val mappedDatatype = getPrimitiveTypeName(datatype);
		if (array) {
			return "QList<" + mappedDatatype + "> ";
		} else {
			return mappedDatatype;
		}
	}
	
	override getDefaultValue(FTypedElement element) {
		//default values are not supported (currently) by the Franca IDL 
		/*if (member.getDEFAULTVALUE()!=null && !member.getDEFAULTVALUE().isEmpty()){
			if (isEnum(member)){
				val ENUMDATATYPETYPE enumDatatype = getDatatype(id) as ENUMDATATYPETYPE
				for (ENUMELEMENTTYPE element : getEnumElements(enumDatatype)){
					if (element.VALUE == member.DEFAULTVALUE){
						return enumDatatype.SHORTNAME.toFirstUpper + "::" + element.SYNONYM
					}
				}
				return getPackagePath(enumDatatype, "::") + "::" + enumDatatype.SHORTNAME.toFirstUpper + "::" +  (enumDatatype.ENUMERATIONELEMENTS.ENUMELEMENT.get(0) as ENUMELEMENTTYPE).SYNONYM
			}
			else if (isLong(member.getDATATYPEREF().getIDREF())){
				return member.getDEFAULTVALUE() + "L"
			}
			else if (isDouble(member.getDATATYPEREF().getIDREF())){
				return member.getDEFAULTVALUE() + "d"
			}
			else{
				return member.getDEFAULTVALUE();
			}
		} else */if (isComplex(element.type)) {
			return "";
		} else if (isArray(element)){
			return "";
		} else if (isEnum(element.type)){
			return " /* should have enum default value here */";
		} else if (!primitiveDataTypeDefaultMap.containsKey(element.type.predefined)) {
 			return "NaN";
 		} else {
			return primitiveDataTypeDefaultMap.get(element.type.predefined);
		}
		
	}

	
	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype){
		val members = getComplexAndEnumMembers(datatype);
		
		val typeList = new TreeSet<String>();
		if (hasExtendsDeclaration(datatype)){
			typeList.add(getIncludeOf(getExtendedType(datatype)))
		}

		for (member : members) {
			val type = getDatatype(member.type);
			if (type instanceof FType){
				typeList.add(getIncludeOf(type));
			}
		}	
		return typeList;
	}
		
	def Iterable<String> getRequiredIncludesFor(FInterface serviceInterface){
		val includeSet = new TreeSet<String>();
		for(datatype: getAllComplexAndEnumTypes(serviceInterface)){
			if (datatype instanceof FType){
				includeSet.add(getIncludeOf(datatype));
			}
		}
		return includeSet;
	}
	
	override String getOneLineWarning() {
		//return ""
		return "/* Generated Code */  "
	}

	// Get the class that encloses a known enum
	def String getEnumContainer(FType enumeration) {
		var packagepath = buildPackagePath(enumeration, "::");
		return packagepath + enumeration.joynrName;
	}

	// Get the class that encloses a known enum
	def String getEnumContainer(FTypeRef enumeration) {
        return getEnumContainer(enumeration.derived);
	}
 
	// Get the name of enum types that are nested in an Enum wrapper class
	def String getNestedEnumName() {
		return "Enum";
	}

	// Convert a data type declaration into a string giving the typename
	def String getJoynrTypeName(FTypedElement element) {
		val datatypeRef = element.type;
		val datatype = datatypeRef.derived;
		val predefined = datatypeRef.predefined;

		switch datatype {
		case isArray(element)     : "List"
		case isEnum(datatypeRef)  : getPackagePathWithJoynrPrefix(datatype, ".") + 
									"." + datatype.joynrName
		case isString(predefined) : "String"
		case isInt(predefined)    : "Integer"
		case isLong(predefined)   : "Long"
		case isDouble(predefined) : "Double"
		case isFloat(predefined)  : "Double"
		case isBool(predefined)   : "Boolean"
		case isByte(predefined)   : "Byte"
		case datatype != null     : getPackagePathWithJoynrPrefix(datatype, ".") + 
									"." + datatype.joynrName
        default                   : throw new RuntimeException("Unhandled primitive type: " + predefined.getName)
		}
	}
	
	// Return a call to a macro that allows classes to be exported and imported
	// from DLLs when compiling with VC++
	def String getDllExportMacro() {
		if (!dllExportName.isEmpty()) {
			return dllExportName.toUpperCase() + "_EXPORT";
		} 
		return "";
	}
	
	// Return an include statement that pulls in VC++ macros for DLL import and
	// export
	def String getDllExportIncludeStatement() {
		if (!dllExportName.isEmpty()) {
			return "#include \"" + joynrGenerationPrefix + "/" + dllExportName + "Export.h\"";
		}
		return "";
	}
	
	def String getIncludeOf(FType dataType) {
		val path = getPackagePathWithJoynrPrefix(dataType, "/")
		return path + "/" + dataType.joynrName + ".h";
	}

	def getPackageSourceDirectory(FModelElement fModelElement) {
    	return super.getPackageName(fModelElement).replace('.', File::separator)
	}
   	
}
