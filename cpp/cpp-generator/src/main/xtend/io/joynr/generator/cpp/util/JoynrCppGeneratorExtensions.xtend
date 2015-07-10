package io.joynr.generator.cpp.util
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

import com.google.common.collect.Iterators
import com.google.inject.Inject
import com.google.inject.name.Named
import io.joynr.generator.util.JoynrGeneratorExtensions
import java.io.File
import java.util.Iterator
import java.util.TreeSet
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement

class JoynrCppGeneratorExtensions extends JoynrGeneratorExtensions {

	@Inject @Named("generationId")
	String dllExportName;

	def String getNamespaceStarter(FInterface interfaceType) {
		getNamespaceStarter(getPackageNames(interfaceType));
	}

	def String getNamespaceStarter(FType datatype) {
		return getNamespaceStarter(datatype, false);
	}

	def String[] getNamespaces(FType datatype, boolean includeTypeCollection) {
				var String packagePath = datatype.getPackagePathWithoutJoynrPrefix(".");
		if (includeTypeCollection && datatype.isPartOfTypeCollection) {
			packagePath += "." + datatype.typeCollectionName;
		}
		return packagePath.split("\\.");
	}

	def String getNamespaceStarter(FType datatype, boolean includeTypeCollection) {
		return getNamespaceStarter(Iterators::forArray(getNamespaces(datatype, includeTypeCollection)));
	}

	def String getNamespaceEnder(FInterface interfaceType) {
		getNamespaceEnder(getPackageNames(interfaceType));
	}

	def String getNamespaceEnder(FType datatype, boolean includeTypeCollection) {
		return getNamespaceEnder(Iterators::forArray(getNamespaces(datatype, includeTypeCollection)));
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

	def buildPackagePath(FType datatype, String separator) {
		return buildPackagePath(datatype, separator, false);
	}

	def buildPackagePath(FType datatype, String separator, boolean includeTypeCollection) {
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
		if (includeTypeCollection && datatype.partOfTypeCollection) {
			packagepath += datatype.typeCollectionName + separator;
		}
		return packagepath;
	}

	def Iterable<String> getRequiredIncludesForStd(FCompoundType datatype){
		val members = getComplexAndEnumMembers(datatype);

		val typeList = new TreeSet<String>();
		if (hasExtendsDeclaration(datatype)){
			typeList.add(getIncludeOfStd(getExtendedType(datatype)))
		}

		for (member : members) {
			val type = getDatatype(member.type);
			if (type instanceof FType){
				//TODO QT: remove this if statement once merged with std enums
				if (isEnum(type)) {
					typeList.add(getIncludeOf(type));
				} else {
					typeList.add(getIncludeOfStd(type));
				}
			}
		}
		return typeList;
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

		for (broadcast: serviceInterface.broadcasts) {
			if (isSelective(broadcast)) {
				includeSet.add(getIncludeOfFilterParametersContainer(serviceInterface, broadcast));
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
			return dllExportName.toUpperCase() + "_EXPORT ";
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

	def joynrNameStd(FType type){
		return "Std" + type.name
	}

	def String getIncludeOf(FType dataType) {
		val path = getPackagePathWithJoynrPrefix(dataType, "/")
		return path + "/" + dataType.joynrName + ".h";
	}

	def String getIncludeOfStd(FType dataType) {
		var path = getPackagePathWithJoynrPrefix(dataType, "/")
		if (dataType.isPartOfTypeCollection) {
			path += "/" + dataType.typeCollectionName
		}
		return path + "/" + dataType.joynrNameStd + ".h";
	}

	def String getIncludeOfFilterParametersContainer(FInterface serviceInterface, FBroadcast broadcast) {
		return getPackagePathWithJoynrPrefix(serviceInterface, "/")
			+ "/" + serviceInterface.name.toFirstUpper 
			+ broadcast.joynrName.toFirstUpper
			+ "BroadcastFilterParameters.h"
	}

	def getPackageSourceDirectory(FModelElement fModelElement) {
		return super.getPackageName(fModelElement).replace('.', File::separator)
	}
}
