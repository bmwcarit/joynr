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

import com.google.inject.Inject
import io.joynr.generator.templates.util.AbstractTypeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import java.util.Collections
import java.util.HashMap
import java.util.HashSet
import java.util.Map
import java.util.Set
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypedElement
import io.joynr.generator.templates.util.InterfaceUtil.TypeSelector

abstract class CppTypeUtil extends AbstractTypeUtil {
	@Inject extension InterfaceUtil
	@Inject extension MethodUtil
	@Inject extension BroadcastUtil
	@Inject protected extension JoynrCppGeneratorExtensions

	Map<FBasicTypeId, String> primitiveDataTypeDefaultMap;

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
		bMap.put(FBasicTypeId::UINT8, "0");
		bMap.put(FBasicTypeId::INT16, "-1");
		bMap.put(FBasicTypeId::UINT16, "0");
		bMap.put(FBasicTypeId::INT32, "-1");
		bMap.put(FBasicTypeId::UINT32, "0");
		bMap.put(FBasicTypeId::INT64, "-1");
		bMap.put(FBasicTypeId::UINT64, "0");
		bMap.put(FBasicTypeId::FLOAT, "-1");
		bMap.put(FBasicTypeId::DOUBLE, "-1");
		bMap.put(FBasicTypeId::STRING, "\"\"");
		bMap.put(FBasicTypeId::BYTE_BUFFER, "\"\"");
		bMap.put(FBasicTypeId::UNDEFINED,"");

		primitiveDataTypeDefaultMap = Collections::unmodifiableMap(bMap);
	}

	def getCommaSeperatedUntypedInputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.inputParameters);
	}

	def getCommaSeperatedUntypedOutputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.outputParameters);
	}

	def getCommaSeperatedUntypedParameterList(Iterable<FArgument> arguments) {
		val returnStringBuilder = new StringBuilder();
		for (argument : arguments) {
			returnStringBuilder.append(argument.joynrName)
			returnStringBuilder.append(", ")
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		}
		return returnString.substring(0, returnString.length() - 2); //remove the last ,
	}

	def String getCommaSeparatedOutputParameterTypes(FMethod method, boolean generateVersion) {
		getCommaSeparatedParameterTypes(method.outputParameters, generateVersion)
	}

	def String getCommaSeparatedOutputParameterTypes(FBroadcast broadcast, boolean generateVersion) {
		getCommaSeparatedParameterTypes(broadcast.outputParameters, generateVersion)
	}

	private def String getCommaSeparatedParameterTypes(Iterable<FArgument> arguments, boolean generateVersion) {
		val commaSeparatedParams = new StringBuilder();
		for (parameter : arguments.mapParametersToTypeName(generateVersion)) {
			commaSeparatedParams.append(parameter);
			commaSeparatedParams.append(", ");
		}
		val returnString = commaSeparatedParams.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	private def getCommaSeperatedTypedParameterList(
		Iterable<FArgument> arguments,
		boolean constParameters,
		boolean parameterAsReference,
		boolean generateVersion
	) {
		val returnStringBuilder = new StringBuilder();
		var firstElement = true;

		for(FArgument argument : arguments){
			if (firstElement) {
				firstElement = false;
			} else {
				returnStringBuilder.append("\n")
			}
			if (constParameters) {
				returnStringBuilder.append("const ");
			}

			returnStringBuilder.append(argument.getTypeName(generateVersion));

			if (parameterAsReference) {
				returnStringBuilder.append("&");
			}

			returnStringBuilder.append(" ");
			returnStringBuilder.append(argument.joynrName);
			returnStringBuilder.append(",");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 1); //remove the last " ," or "\n,"
		}
	}

	def getCommaSeperatedTypedOutputParameterList(FMethod method, boolean generateVersion) {
		getCommaSeperatedTypedParameterList(method.outputParameters, false, true, generateVersion)
	}

	def getCommaSeperatedTypedConstOutputParameterList(FMethod method, boolean generateVersion) {
		getCommaSeperatedTypedParameterList(method.outputParameters, true, true, generateVersion)
	}

	def getCommaSeperatedTypedOutputParameterList(FBroadcast broadcast, boolean generateVersion) {
		getCommaSeperatedTypedParameterList(broadcast.outputParameters, false, true, generateVersion)
	}

	def getCommaSeperatedTypedConstOutputParameterList(FBroadcast broadcast, boolean generateVersion) {
		getCommaSeperatedTypedParameterList(broadcast.outputParameters, true, true, generateVersion)
	}

	def getCommaSeperatedTypedConstInputParameterList(FMethod method, boolean generateVersion) {
		getCommaSeperatedTypedParameterList(method.inputParameters, true, true, generateVersion)
	}

	def getDefaultValue(FTypedElement element, boolean generateVersion) {
		//default values are not supported (currently) by the Franca IDL 
		/*if (member.getDEFAULTVALUE()!==null && !member.getDEFAULTVALUE().isEmpty()){
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
		} else */
		if (isCompound(element.type) || element.type.isMap) {
			return "";
		} else if (isArray(element)){
			return "";
		} else if (isEnum(element.type)){
			val path = element.type.enumType.buildPackagePath("::", true, generateVersion) + "::" + element.type.enumType.joynrName;
			return path + "::" + element.type.enumType.enumerators.get(0).joynrName;
		} else if (!primitiveDataTypeDefaultMap.containsKey(element.type.predefined)) {
 			return "NaN";
 		} else {
			return primitiveDataTypeDefaultMap.get(element.type.predefined);
		}
	}

	def Iterable<FType> getTypeDependencies(FCompoundType datatype){
		val members = getComplexMembers(datatype, true);

		val typeList = new HashSet<FType>();
		if (hasExtendsDeclaration(datatype)){
			typeList.add(datatype.extendedType)
		}

		for (member : members) {
			val type = getDatatype(member.type);
			if (type instanceof FType){
				typeList.add(type);
			}
		}
		return typeList;
	}

	def Iterable<? extends Object> getTypeDependencies(FMapType datatype){
		val typeList = new HashSet<Object>();
		var type = getDatatype(datatype.keyType);
		if (type instanceof FType || type instanceof FBasicTypeId){
			typeList.add(type);
		}

		type = getDatatype(datatype.valueType)
		if (type instanceof FType || type instanceof FBasicTypeId){
			typeList.add(type);
		}

		return typeList;
	}

	def Iterable<? extends Object> getTypeDependencies(FTypeDef datatype){
		val typeList = new HashSet<Object>();
		var type = getDatatype(datatype.actualType);
		if (type instanceof FType){
			typeList.add(type);
		} else if (type instanceof FBasicTypeId){
			typeList.addAll(type)
		}

		return typeList;
	}

	def Set<String> getIncludesFor(Iterable<FBasicTypeId> datatypes)

	abstract def String getIncludeForArray()

	abstract def String getIncludeOf(FType type, boolean generateVersioning)

	abstract def String getIncludeForString()

	override getDatatype(FType type){
		if (type.isTypeDef) {
			return type
		}
		return super.getDatatype(type)
	}

	def Set<String> getDataTypeIncludesFor(FInterface serviceInterface, boolean generateVersioning){
		val includeSet = new HashSet<String>();
		val selector = TypeSelector::defaultTypeSelector
		selector.errorTypes(true)
		selector.typeDefs(true)
		for(datatype: getAllComplexTypes(serviceInterface,selector)){
			includeSet.add(datatype.getIncludeOf(generateVersioning));
		}

		includeSet.addAll(serviceInterface.allPrimitiveTypes.includesFor)
		if (serviceInterface.hasArray && includeForArray !== null){
			includeSet.add(includeForArray)
		}
		if (!serviceInterface.broadcasts.filter[!selective].empty) {
			includeSet.add(includeForArray)
			includeSet.add(includeForString)
		}
		return includeSet
	}

	def Set<String> getBroadcastFilterParametersClassNames(FInterface serviceInterface){
		val classNameSet = new HashSet<String>();
		for (broadcast: serviceInterface.broadcasts) {
			if (broadcast.selective) {
				classNameSet.add(
					serviceInterface.name.toFirstUpper +
					broadcast.joynrName.toFirstUpper +
					"BroadcastFilterParameters");
			}
		}
		return classNameSet
	}

	def Set<String> getBroadcastFilterParametersIncludes(FInterface serviceInterface, boolean generateVersion){
		val includeSet = new HashSet<String>();
		for (broadcast: serviceInterface.broadcasts) {
			if (broadcast.selective) {
				includeSet.add(getIncludeOfFilterParametersContainer(serviceInterface, broadcast, generateVersion));
			}
		}
		return includeSet
	}


	abstract def String getGenerationTypeName (FType datatype);

	def getTypeNameOfContainingClass (FType datatype, boolean generateVersion) {
		val packagepath = buildPackagePath(datatype, "::", true, generateVersion);
		return  packagepath + "::" + datatype.generationTypeName
	}
}
