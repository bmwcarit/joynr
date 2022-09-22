package io.joynr.generator.util

import com.google.inject.Inject
import io.joynr.generator.templates.util.AbstractTypeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.MethodUtil
import java.util.Collections
import java.util.HashMap
import java.util.Map
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement

class JavaTypeUtil extends AbstractTypeUtil {

	@Inject extension MethodUtil
	@Inject extension BroadcastUtil

	Map<FBasicTypeId, String> primitiveDataTypeDefaultMap;

	new () {
	val Map<FBasicTypeId,String> primitiveDataTypeDefaultValue = new HashMap<FBasicTypeId, String>();
		primitiveDataTypeDefaultValue.put(FBasicTypeId::BOOLEAN, "false");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT8, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT8, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT16, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT16, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT32, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT32, "0");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::INT64, "0L");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UINT64, "0l");
		//see bug JOYN-1521: floats are interpreted as double
		primitiveDataTypeDefaultValue.put(FBasicTypeId::FLOAT, "0f");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::DOUBLE, "0d");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::STRING, "\"\"");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::BYTE_BUFFER, "new Byte[0]");
		primitiveDataTypeDefaultValue.put(FBasicTypeId::UNDEFINED,"");

		primitiveDataTypeDefaultMap = Collections::unmodifiableMap(primitiveDataTypeDefaultValue);
	}

	def getCommaSeperatedTypedOutputParameterList(
		Iterable<FArgument> arguments,
		boolean linebreak
	) {
		val returnStringBuilder = new StringBuilder();
		for (FArgument argument : arguments) {

			returnStringBuilder.append(argument.typeName);
			returnStringBuilder.append(" ");
			returnStringBuilder.append(argument.joynrName);
			returnStringBuilder.append(",");

			if (linebreak) {
				returnStringBuilder.append("\n");
			} else {
				returnStringBuilder.append(" ");
			}
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last " ," or "\n,"
		}
	}

	def getCommaSeperatedTypedOutputParameterList(FMethod method) {
		return getCommaSeperatedTypedOutputParameterList(getOutputParameters(method), false)
	}

	def getCommaSeperatedTypedOutputParameterList(FBroadcast broadcast) {
		return getCommaSeperatedTypedOutputParameterList(getOutputParameters(broadcast), false)
	}

	def getCommaSeperatedTypedOutputParameterListLinebreak(FBroadcast broadcast) {
		return getCommaSeperatedTypedOutputParameterList(getOutputParameters(broadcast), true)
	}

	def getCommaSeperatedUntypedInputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.inputParameters);
	}

	def getCommaSeperatedUntypedOutputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.outputParameters);
	}

	def getCommaSeperatedUntypedOutputParameterList(FBroadcast broadcast) {
		getCommaSeperatedUntypedParameterList(broadcast.outputParameters);
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

	def getCommaSeperatedTypedParameterList(FMethod method) {
		val returnStringBuilder = new StringBuilder();
		for (param : getInputParameters(method)) {
			returnStringBuilder.append(param.typeName);
			returnStringBuilder.append(" ");
			returnStringBuilder.append(param.joynrName);
			returnStringBuilder.append(", ");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	def getCommaSeperatedTypedFilterParameterList(FBroadcast broadcast) {
		val returnStringBuilder = new StringBuilder();
		for (filterParameter : getFilterParameters(broadcast)) {
			returnStringBuilder.append("String ");
			returnStringBuilder.append(filterParameter);
			returnStringBuilder.append(", ");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	override isCompound(FType type) {
		if (type instanceof FTypeDef) {
			return isCompound(type.actualType)
		}
		return super.isCompound(type)
	}

	override getCompoundType(FType type) {
		if (type instanceof FTypeDef) {
			return getCompoundType(type.actualType)
		}
		return super.getCompoundType(type)
	}

	override isPrimitive(FType type) {
		if (type instanceof FTypeDef) {
			return isPrimitive(type.actualType)
		}
		return super.isPrimitive(type)
	}

	override getPrimitive(FType type) {
		if (type instanceof FTypeDef) {
			return getPrimitive(type.actualType)
		}
		return super.getPrimitive(type)
	}

	override getTypeName(FBasicTypeId datatype) {
		switch datatype {
			case FBasicTypeId::BOOLEAN: "Boolean"
			case FBasicTypeId::INT8: "Byte"
			case FBasicTypeId::UINT8: "Byte"
			case FBasicTypeId::INT16: "Short"
			case FBasicTypeId::UINT16: "Short"
			case FBasicTypeId::INT32: "Integer"
			case FBasicTypeId::UINT32: "Integer"
			case FBasicTypeId::INT64: "Long"
			case FBasicTypeId::UINT64: "Long"
			case FBasicTypeId::FLOAT: "Float"
			case FBasicTypeId::DOUBLE: "Double"
			case FBasicTypeId::STRING: "String"
			case FBasicTypeId::BYTE_BUFFER: "Byte[]"
			default: throw new IllegalArgumentException("Unsupported basic type: " + datatype.getName)
		}
	}

	override getTypeName(FType datatype) {
		if (isEnum(datatype)) {
			return datatype.enumType.joynrName;
		}
		if (isPrimitive(datatype)) {
			return datatype.getPrimitive.typeName
		}
		if (isCompound(datatype)) {
			return datatype.compoundType.joynrName
		}
		if (isMap(datatype)) {
			return datatype.mapType.joynrName
		}
		throw new IllegalStateException("JavaTypeUtil.getTypeName: unsupported state, datatype " +
			datatype.joynrName + " could not be mapped to an implementation datatype")
	}

	override getTypeName(FType datatype, boolean generateVersion) {
		throw new IllegalStateException("Unsupported method called for Java!")
	}

	override getTypeNameForList(FType datatype, boolean generateVersion) {
		throw new IllegalStateException("Unsupported method called for Java!")
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		getObjectDataTypeForPlainType(datatype.typeName) + "[]";
	}

	override getTypeNameForList(FType datatype) {
		getObjectDataTypeForPlainType(datatype.typeName) + "[]";
	}

	def String getTypedParameterList(Iterable<FArgument> params) {
		var sb = new StringBuilder();
		var i = 0;
		while (i < params.size) {
			val param = params.get(i);
			sb.append(param.typeName + " "+ param.joynrName)
			if (i != params.size-1) {
				sb.append(",\n")
			}
			i = i+1;
		}
		return sb.toString
	}

	def String getJavadocCommentsParameterListJavaRpc(FMethod method) {
		var sb = new StringBuilder();
		val params = getInputParameters(method)
		var i = 0;
		while (i < params.size) {
			val param = params.get(i);
			sb.append(" * @param " + param.joynrName + " the parameter " + param.joynrName + "\n");
			i = i+1;
		}
		return sb.toString
	}

	def getDefaultValue(FTypedElement element) {
		if ((isArray(element))) {
			return "{ }";
		}
		return element.type.getDefaultValue();
	}

	def String getDefaultValue(FTypeRef typeRef) {
		if (typeRef.isTypeDef) {
			return getDefaultValue(typeRef.typeDefType.actualType);
		}
		if (typeRef.isMap) {
			return "new " + typeRef.joynrName + "()";
		}
		if (typeRef.isCompound || typeRef.isMap) {
			return "new " + typeRef.compoundType.joynrName + "()";
		} else if (typeRef.isEnum) {
			return typeRef.enumType.joynrName + "." + typeRef.enumType.enumerators.get(0).joynrName;
		} else if (!primitiveDataTypeDefaultMap.containsKey(typeRef.predefined)) {
 			return "NaN";
 		} else if (typeRef.isPrimitive) {
			return primitiveDataTypeDefaultMap.get(typeRef.predefined);
		}
	}
}
