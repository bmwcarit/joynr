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

import com.google.inject.Inject
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import java.util.Iterator
import java.util.TreeSet
import org.franca.core.franca.FAnnotation
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType

class JoynrJavaGeneratorExtensions extends io.joynr.generator.templates.util.JoynrGeneratorExtensions {
	@Inject extension JavaTypeUtil
	@Inject extension InterfaceUtil
	@Inject extension BroadcastUtil

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
			if (includeTypeCollection && datatype.partOfTypeCollection) {
				packagepath += separator + datatype.typeCollectionName;
			}
		};
		return packagepath;
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

	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype){
		getRequiredIncludesFor(datatype, true);
	}

	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype, boolean includingExendedType){
		val members = getComplexMembers(datatype);

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
				typeList.add(getIncludeOf(type));
			}
		}
		return typeList;
	}

	def Iterable<String> getRequiredIncludesFor(FInterface serviceInterface) {
		getRequiredIncludesFor(serviceInterface, true, true, true, true, true);
	}

	def Iterable<String> getRequiredIncludesFor(
			FInterface serviceInterface,
			boolean methods,
			boolean readAttributes,
			boolean writeAttributes,
			boolean notifyAttributes,
			boolean broadcasts
	) {
		val includeSet = new TreeSet<String>();
		for(datatype : getAllComplexTypes(serviceInterface, methods, readAttributes, writeAttributes, notifyAttributes, broadcasts)) {
			if (datatype instanceof FType){
				val include = getIncludeOf(datatype);
				if (include != null) {
					includeSet.add(include);
				}
			}
//			else{
//				includeSet.add(getIncludeOf(datatype as FBasicTypeId));
//			}
		}
		return includeSet;
	}

	def Iterable<String> getRequiredIncludesFor(FBroadcast broadcast) {
		val includeSet = new TreeSet<String>();
		for(datatype: getAllComplexTypes(broadcast)) {
			if (datatype instanceof FType) {
				includeSet.add(getIncludeOf(datatype));
			}
		}
		return includeSet;
	}

	def ReformatComment(FAnnotation comment, String prefixForNewLines) {
		return comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefixForNewLines)
	}

	// for classes and methods
	def appendJavadocSummaryAndWriteSeeAndDescription(FModelElement element, String prefixForNewLines)'''
		«IF element.comment != null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«prefixForNewLines» «ReformatComment(comment, prefixForNewLines)»
				«ENDIF»
			«ENDFOR»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::SEE»
					«prefixForNewLines» @see «ReformatComment(comment, prefixForNewLines)»
				«ENDIF»
				«IF comment.type == FAnnotationType::DETAILS»
					«prefixForNewLines»
					«prefixForNewLines» «ReformatComment(comment, prefixForNewLines)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	// for parts
	def appendJavadocComment(FModelElement element, String prefixForNewLines)'''
		«IF element.comment != null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«ReformatComment(comment, prefixForNewLines)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	// for parameters
	def appendJavadocParameter(FModelElement element, String prefixForNewLines)'''
		«IF element.comment != null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«prefixForNewLines» @param «element.joynrName» «ReformatComment(comment, prefixForNewLines)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	def String getIncludeOf(FType dataType) {
		return dataType.buildPackagePath(".", true) + "." + dataType.joynrName;
	}

	override String getOneLineWarning() {
		//return ""
		return "/* Generated Code */  "
	}

	// Returns true if a class has to create lists in its constructor
	def boolean hasArrayMembers(FCompoundType datatype){
		for (member : datatype.members) {
			if (isArray(member)){
				return true
			}
		}
		return false
	}

	def getJoynTypePackagePrefix(){
		joynrGenerationPrefix
	}
}
