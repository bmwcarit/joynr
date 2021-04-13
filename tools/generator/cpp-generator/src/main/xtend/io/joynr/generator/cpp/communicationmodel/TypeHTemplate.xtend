package io.joynr.generator.cpp.communicationmodel
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

import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.CompoundTypeTemplate
import io.joynr.generator.templates.util.NamingUtil
import javax.inject.Inject
import org.franca.core.franca.FCompoundType
import com.google.inject.assistedinject.Assisted

class TypeHTemplate extends CompoundTypeTemplate {

	@Inject extension JoynrCppGeneratorExtensions

	@Inject extension CppStdTypeUtil

	@Inject extension NamingUtil

	@Inject extension TemplateBase

	@Inject
	new(@Assisted FCompoundType type) {
		super(type)
	}

	override generate(boolean generateVersion)
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_", true, generateVersion)+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>

#include "joynr/Util.h"
#include "joynr/ByteBuffer.h"

// include complex Datatype headers.
«FOR member: type.typeDependencies»
	#include «member.getIncludeOf(generateVersion)»
«ENDFOR»

#include "joynr/serializer/Serializer.h"

«getNamespaceStarter(type, true, generateVersion)»

/**
«appendDoxygenSummaryAndWriteSeeAndDescription(type, " *")»
 * @version «majorVersion».«minorVersion»
 */
class «getDllExportMacro()» «typeName» «IF hasExtendsDeclaration(type)»: public «getExtendedType(type).getTypeName(generateVersion)»«ENDIF»{

public:
	/**
	 * @brief MAJOR_VERSION The major version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::int32_t MAJOR_VERSION;
	/**
	 * @brief MINOR_VERSION The minor version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::int32_t MINOR_VERSION;

	// general methods

	// default constructor
	/** @brief Constructor */
	«typeName»();

	// constructor setting all fields
	«IF !getMembersRecursive(type).empty»
	/**
	 * @brief Parameterized constructor
	 «FOR member: getMembersRecursive(type)»
	 «appendDoxygenParameter(member, "*")»
	 «ENDFOR»
	 */
	explicit «typeName»(
			«FOR member: getMembersRecursive(type) SEPARATOR","»
				const «member.getTypeName(generateVersion)»& _«member.joynrName»
			«ENDFOR»
	);
	«ENDIF»

	/** @brief Copy constructor */
	«typeName»(const «typeName»&) = default;

	/** @brief Move constructor */
	«typeName»(«typeName»&&) = default;

	/** @brief Destructor */
	«IF isPolymorphic(type)»
		«IF !hasExtendsDeclaration(type)»
		virtual ~«typeName»() = default;
		«ELSE»
		~«typeName»() override = default;
		«ENDIF»
	«ELSE»
	~«typeName»() = default;
	«ENDIF»

	/**
	 * @brief Stringifies the class
	 * @return stringified class content
	 */
	«IF isPolymorphic(type)»
		«IF !hasExtendsDeclaration(type)»
		virtual std::string toString() const;
		«ELSE»
		std::string toString() const override;
		«ENDIF»
	«ELSE»
	std::string toString() const;
	«ENDIF»

	/**
	 * @brief Returns a hash code value for this object
	 * @return a hash code value for this object.
	 */
	«IF isPolymorphic(type)»
		«IF !hasExtendsDeclaration(type)»
		virtual std::size_t hashCode() const;
		«ELSE»
		std::size_t hashCode() const override;
		«ENDIF»
	«ELSE»
	std::size_t hashCode() const;
	«ENDIF»

	/**
	 * @brief assigns an object
	 * @return reference to the object assigned to
	 */
	«typeName»& operator=(const «typeName»&) = default;

	/**
	 * @brief move assigns an object
	 * @return reference to the object assigned to
	 */
	«typeName»& operator=(«typeName»&&) = default;

	«IF !hasExtendsDeclaration(type) || !isPolymorphic(type)»
	/**
	 * @brief "equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool operator==(const «typeName»& other) const
	{
		return this->equals(other, joynr::util::MAX_ULPS);
	}

	/**
	 * @brief "not equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	bool operator!=(const «typeName»& other) const
	{
		return !(*this == other);
	}
	«ENDIF»

	«IF isPolymorphic(type)»
	/**
	 * @return a copy of this object
	 */
	 «IF !hasExtendsDeclaration(type)»
	 std::unique_ptr<«getRootType(type).getTypeName(generateVersion)»> virtual clone() const;
	 «ELSE»
	 std::unique_ptr<«getRootType(type).getTypeName(generateVersion)»> clone() const override;
	 «ENDIF»
	«ENDIF»

	// getters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		/**
		 * @brief Gets «joynrName.toFirstUpper»
		 * @return «appendDoxygenComment(member, "* ")»
		 */
		inline const «member.getTypeName(generateVersion)»& get«joynrName.toFirstUpper»() const { return «joynrName»; }
	«ENDFOR»

	// setters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		/**
		 * @brief Sets «joynrName.toFirstUpper»
		 «appendDoxygenParameter(member, "*")»
		 */
		inline void set«joynrName.toFirstUpper»(const «member.getTypeName(generateVersion)»& _«joynrName») { this->«joynrName» = _«joynrName»; }
	«ENDFOR»

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @param maxUlps maximum number of ULPs (Units in the Last Place) that are tolerated when comparing to floating point values
	 * @return true if objects are equal, false otherwise
	 */
	«IF isPolymorphic(type) && !hasExtendsDeclaration(type)»virtual «ENDIF»bool equals(const «IF isPolymorphic(type)»«getRootType(type).getTypeName(generateVersion)»«ELSE»«typeName»«ENDIF»& other, std::size_t maxUlps) const«IF isPolymorphic(type) && hasExtendsDeclaration(type)» override«ENDIF»
	{
	«IF isPolymorphic(type)»
		if (typeid(*this) != typeid(other)) {
			return false;
		}
	«ENDIF»
		return this->equalsInternal(other, maxUlps);
	}
protected:
	// printing «typeName» with google-test and google-mock
	/**
	 * @brief Print values of a «typeName» object
	 * @param «typeName.toFirstLower» The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const «typeName»& «typeName.toFirstLower», ::std::ostream* os);

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	«IF hasExtendsDeclaration(type)»
		bool equalsInternal(const «IF isPolymorphic(type)»«getRootType(type).getTypeName(generateVersion)»& otherBase«ELSE»«typeName»& other«ENDIF», std::size_t maxUlps) const«IF isPolymorphic(type)» override«ENDIF»
		{
			«IF isPolymorphic(type)»
			const «typeName»& other = static_cast<const «typeName»&>(otherBase);
			«ENDIF»
			«IF getMembers(type).size > 0»
				return
				«FOR member: getMembers(type) SEPARATOR '&&'»
					joynr::util::compareValues(this->«member.joynrName», other.«member.joynrName», maxUlps)
				«ENDFOR»
				&& «getExtendedType(type).joynrName»::equalsInternal(other, maxUlps);
			«ELSE»
				return «getExtendedType(type).joynrName»::equalsInternal(other, maxUlps);
			«ENDIF»
		}
	«ELSE»
		«IF isPolymorphic(type)»virtual «ENDIF»bool equalsInternal(const «typeName»& other, std::size_t maxUlps) const
		{
			«IF getMembers(type).size > 0»
				return
				«FOR member: getMembers(type) SEPARATOR ' &&'»
					joynr::util::compareValues(this->«member.joynrName», other.«member.joynrName», maxUlps)
				«ENDFOR»
				;
			«ELSE»
				std::ignore = other;
				std::ignore = maxUlps;
				return true;
			«ENDIF»
		}
	«ENDIF»


«val serializeObjName = typeName.toLowerCase + "Obj"»
private:
	// serialize «typeName» with muesli
	template <typename Archive>
	friend void serialize(Archive& archive, «typeName»& «serializeObjName»);

	// members
	«FOR member: getMembers(type)»
		«member.getTypeName(generateVersion)» «member.joynrName»;
		«IF isEnum(member.type)»
			std::string get«member.joynrName.toFirstUpper»Internal() const;
		«ENDIF»
	«ENDFOR»
};

std::size_t hash_value(const «typeName»& «typeName.toFirstLower»Value);

// serialize «typeName» with muesli
template <typename Archive>
void serialize(Archive& archive, «typeName»& «serializeObjName»)
{
«IF getMembers(type).size > 0 || hasExtendsDeclaration(type)»
	archive(
			«IF hasExtendsDeclaration(type)»
			muesli::BaseClass<«getExtendedType(type).getTypeName(generateVersion)»>(&«serializeObjName»)«IF type.members.size >0 »,«ENDIF»
			«ENDIF»
			«FOR member: type.members SEPARATOR ','»
			muesli::make_nvp("«member.joynrName»", «serializeObjName».«member.joynrName»)
			«ENDFOR»
	);
«ELSE»
	std::ignore = archive;
	std::ignore = «serializeObjName»;
«ENDIF»
}

«getNamespaceEnder(type, true, generateVersion)»

namespace std {

/**
 * @brief Function object that implements a hash function for «type.getTypeName(generateVersion)».
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<«type.getTypeName(generateVersion)»> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param «typeName.toFirstLower»Value the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const «type.getTypeName(generateVersion)»& «typeName.toFirstLower»Value) const {
		return «type.buildPackagePath("::", true, generateVersion)»::hash_value(«typeName.toFirstLower»Value);
	}
};
} // namespace std

«val typeNameString = type.getTypeName(generateVersion).replace("::", ".")»
«IF type.hasExtendsDeclaration && isPolymorphic(type)»
MUESLI_REGISTER_POLYMORPHIC_TYPE(«type.getTypeName(generateVersion)», «getExtendedType(type).getTypeName(generateVersion)», "«typeNameString»")
«ELSE»
MUESLI_REGISTER_TYPE(«type.getTypeName(generateVersion)», "«typeNameString»")
«ENDIF»

#endif // «headerGuard»
'''
}
