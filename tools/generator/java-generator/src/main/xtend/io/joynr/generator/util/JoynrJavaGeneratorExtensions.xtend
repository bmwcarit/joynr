package io.joynr.generator.util
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

import com.google.inject.Inject
import com.google.inject.Singleton
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.InterfaceUtil.TypeSelector
import io.joynr.generator.templates.util.JoynrGeneratorExtensions
import java.util.Iterator
import java.util.TreeSet
import org.franca.core.franca.FAnnotation
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType
import org.franca.core.franca.FTypedElement

@Singleton
class JoynrJavaGeneratorExtensions extends JoynrGeneratorExtensions {
	@Inject extension JavaTypeUtil
	@Inject extension InterfaceUtil
	@Inject extension BroadcastUtil

	var ignoreInvalidNullClassMembersExtension = false

	def String getNamespaceStarter(FInterface interfaceType, boolean generateVersion) {
		getNamespaceStarter(getPackageNames(interfaceType, generateVersion));
	}

	def String getNamespaceStarter(FType datatype, boolean generateVersion) {
		getNamespaceStarter(getPackageNames(datatype, generateVersion));
	}

	def String getNamespaceEnder(FInterface interfaceType, boolean generateVersion) {
		getNamespaceEnder(getPackageNames(interfaceType, generateVersion));
	}

	def String getNamespaceEnder(FType datatype, boolean generateVersion) {
		getNamespaceEnder(getPackageNames(datatype, generateVersion));
	}

	def private String getNamespaceStarter(Iterator<String> packageList) {
		return getNamespaceStarterFromPackageList(packageList);
	}

	def String getNamespaceStarterFromPackageList(Iterator<String> packageList) {
		var sb = new StringBuilder();
		while(packageList.hasNext) {
			sb.append("namespace " + packageList.next + "{ " );
		}
		return sb.toString();
	}

	def private String getNamespaceEnder(Iterator<String> packageList) {
		return getNameSpaceEnderFromPackageList(packageList);
	}

	def String getNameSpaceEnderFromPackageList(Iterator<String> packageList) {
		var sb = new StringBuilder();
		while(packageList.hasNext) {
			sb.insert(0, "} /* namespace " + packageList.next + " */ " );
		}
		return sb.toString();
	}

	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype, boolean generateVersion) {
		getRequiredIncludesFor(datatype, true, generateVersion);
	}

	def Iterable<String> getRequiredIncludesFor(FCompoundType datatype, boolean includingExendedType, boolean generateVersion) {
		val members = getComplexMembers(datatype);

		val typeList = new TreeSet<String>();
		if (hasExtendsDeclaration(datatype)) {
			if (includingExendedType) {
				typeList.add(getIncludeOf(getExtendedType(datatype), generateVersion))
			}

			typeList.addAll(getRequiredIncludesFor(getExtendedType(datatype), false, generateVersion))
		}

		for (member : members) {
			val type = getDatatype(member.type);
			if (type instanceof FType) {
				typeList.add(getIncludeOf(type, generateVersion));
			}
		}
		return typeList;
	}

	def Iterable<String> getRequiredIncludesFor(FInterface serviceInterface, boolean generateVersion) {
		getRequiredIncludesFor(serviceInterface, true, true, true, true, true, true, generateVersion);
	}

	def Iterable<String> getRequiredIncludesFor(
			FInterface serviceInterface,
			boolean methods,
			boolean readAttributes,
			boolean writeAttributes,
			boolean notifyAttributes,
			boolean broadcasts,
			boolean fireAndForget,
			boolean generateVersion
	) {
		val includeSet = new TreeSet<String>();
		val selector = TypeSelector::defaultTypeSelector
		selector.methods(methods);
		selector.fireAndForget(fireAndForget);
		selector.readAttributes(readAttributes);
		selector.writeAttributes(writeAttributes);
		selector.notifyAttributes(notifyAttributes);
		selector.broadcasts(broadcasts);

		for (datatype : getAllComplexTypes(serviceInterface, selector)) {
			val include = getIncludeOf(datatype, generateVersion);
			if (include !== null) {
				includeSet.add(include);
			}
		}
		return includeSet;
	}

	def Iterable<String> getRequiredIncludesFor(FBroadcast broadcast, boolean generateVersion) {
		val includeSet = new TreeSet<String>();
		for (datatype: getAllComplexTypes(broadcast)) {
			includeSet.add(getIncludeOf(datatype, generateVersion));
		}
		return includeSet;
	}

	def Iterable<String> getRequiredStatelessAsyncIncludesFor(FInterface serviceInterface, boolean generateVersion) {
		val includeSet = new TreeSet<String>()
		for (datatype : getAllComplexStatelessAsyncTypes(serviceInterface, true, false)) {
			val include = getIncludeOf(datatype, generateVersion)
			if (include !== null) {
				includeSet.add(include)
			}
		}
		includeSet
	}

	def Iterable<String> getRequiredStatelessAsyncCallbackIncludesFor(FInterface serviceInterface, boolean generateVersion) {
		val includeSet = new TreeSet<String>()
		for (datatype : getAllComplexStatelessAsyncTypes(serviceInterface, false, true)) {
			val include = getIncludeOf(datatype, generateVersion)
			if (include !== null) {
				includeSet.add(include)
			}
		}
		includeSet
	}

	def ReformatComment(FAnnotation comment, String prefixForNewLines) {
		return comment.comment.replaceAll("\\s+", " ").replaceAll("\n", "\n" + prefixForNewLines)
	}

	// for classes and methods
	def appendJavadocSummaryAndWriteSeeAndDescription(FModelElement element, String prefixForNewLines)'''
		«IF element.comment !== null»
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
		«getElementDescription(element, prefixForNewLines)»
	'''

	def String getElementDescription(FModelElement element, String prefixForNewLines) {
		var description = "";
		var found = false;
		if (element.comment !== null) {
			for (comment : element.comment.elements) {
				if (comment.type == FAnnotationType::DESCRIPTION) {
					description += ReformatComment(comment, prefixForNewLines)
					found = true
				}
			}
		}
		if (!found) {
			description += "description missing in Franca model."
		}
		return description;
	}

	def appendJavadocParameter(FTypedElement element, String prefixForNewLines) {
		var description = prefixForNewLines + " @param " + element.joynrName + " ";
		description += getElementDescription(element, prefixForNewLines);
		if (element.type.isTypeDef) {
			description += "\n" + prefixForNewLines +
			" (type resolved from modeled Franca typedef " + 
			element.type.joynrName +
			" as " +
			element.type.typeDefType.actualType.typeName +
			")";
		}

		return description;
	}

	def String getIncludeOf(FType dataType, boolean generateVersion) {
		return dataType.buildPackagePath(".", true, generateVersion) + "." + dataType.joynrName;
	}

	// Returns true if a class has to create lists in its constructor
	def boolean hasArrayMembers(FCompoundType datatype) {
		for (member : datatype.members) {
			if (isArray(member)) {
				return true
			}
		}
		return false
	}

	def getJoynTypePackagePrefix() {
		joynrGenerationPrefix
	}

	def activateIgnoreInvalidNullClassMembersExtension() {
		ignoreInvalidNullClassMembersExtension = true
	}

	def ignoreInvalidNullClassMembersExtension() {
		ignoreInvalidNullClassMembersExtension
	}

	def getProviderClassName(FInterface francaIntf) {
		francaIntf.joynrName + "Provider"
	}

	def getProxyClassName(FInterface francaIntf) {
		francaIntf.joynrName + "Proxy"
	}

}
