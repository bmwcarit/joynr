package io.joynr.generator.cpp.util
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

import com.google.common.collect.Iterators
import com.google.inject.Inject
import com.google.inject.name.Named
import io.joynr.generator.templates.util.JoynrGeneratorExtensions
import java.io.File
import java.util.Iterator
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType

class JoynrCppGeneratorExtensions extends JoynrGeneratorExtensions {

	@Inject @Named("generationId")
	String dllExportName;

	def String getNamespaceStarter(FInterface interfaceType, boolean generateVersion) {
		getNamespaceStarter(getPackageNames(interfaceType, generateVersion));
	}

	def String getNamespaceStarter(FType datatype, boolean generateVersion) {
		return getNamespaceStarter(datatype, true, generateVersion);
	}

	def String[] getNamespaces(FType datatype, boolean includeTypeCollection, boolean generateVersion) {
				var String packagePath = datatype.getPackagePathWithoutJoynrPrefix(".", generateVersion);
		if (includeTypeCollection && datatype.isPartOfNamedTypeCollection) {
			packagePath += "." + datatype.typeCollectionName;
		}
		return packagePath.split("\\.");
	}

	def String getNamespaceStarter(FType datatype, boolean includeTypeCollection, boolean generateVersion) {
		return getNamespaceStarter(Iterators::forArray(getNamespaces(datatype, includeTypeCollection, generateVersion)));
	}

	def String getNamespaceEnder(FInterface interfaceType, boolean generateVersion) {
		getNameSpaceEnderFromPackageList(getPackageNames(interfaceType, generateVersion));
	}

	def String getNamespaceEnder(FType datatype, boolean includeTypeCollection, boolean generateVersion) {
		return getNameSpaceEnderFromPackageList(Iterators::forArray(getNamespaces(datatype, includeTypeCollection, generateVersion)));
	}

	def String getNamespaceEnder(FType datatype, boolean generateVersion) {
		getNamespaceEnder(datatype, true, generateVersion);
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

	def String getNameSpaceEnderFromPackageList(Iterator<String> packageList){
		var sb = new StringBuilder();
		sb.append("\n} // namespace " + joynrGenerationPrefix);
		while(packageList.hasNext){
			sb.insert(0, "\n} // namespace " + packageList.next);
		}
		return sb.toString();
	}

	// for classes and methods
	def appendDoxygenSummaryAndWriteSeeAndDescription(FModelElement element, String prefix)'''
		«IF element.comment !== null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«prefix» @brief «comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefix)»
				«ENDIF»
			«ENDFOR»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::SEE»
					«prefix» @see «comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefix)»
				«ENDIF»
				«IF comment.type == FAnnotationType::DETAILS»
					«prefix»
					«prefix» «comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefix)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	// for parts
	def appendDoxygenComment(FModelElement element, String prefix)'''
		«IF element.comment !== null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefix)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	// for parameters
	def appendDoxygenParameter(FModelElement element, String prefix)'''
		«IF element.comment !== null»
			«FOR comment : element.comment.elements»
				«IF comment.type == FAnnotationType::DESCRIPTION»
					«prefix» @param «element.joynrName» «comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefix)»
				«ENDIF»
			«ENDFOR»
		«ENDIF»
	'''

	// Get the name of enum types that are nested in an Enum wrapper class
	def String getNestedEnumName() {
		return "Enum";
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

	def String getIncludeOfFilterParametersContainer(FInterface serviceInterface, FBroadcast broadcast, boolean generateVersion) {
		return "\"" + getPackagePathWithJoynrPrefix(serviceInterface, "/", generateVersion)
			+ "/" + serviceInterface.name.toFirstUpper
			+ broadcast.joynrName.toFirstUpper
			+ "BroadcastFilterParameters.h\""
	}

	def getPackageSourceDirectory(FModelElement fModelElement, boolean generateVersion) {
		return super.getPackageName(fModelElement, generateVersion).replace('.', File::separator)
	}
}
