package io.joynr.generator.cpp.communicationmodel
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

import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.QtTypeUtil
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.util.CompoundTypeTemplate
import javax.inject.Inject
import org.franca.core.franca.FCompoundType

class TypeHTemplate implements CompoundTypeTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension QtTypeUtil

	@Inject
	private CppStdTypeUtil stdTypeUtil

	@Inject
	private extension TemplateBase

	override generate(FCompoundType type)
'''
«val typeName = type.joynrNameQt»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_")+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

#include <QObject>
#include <QVariantMap>
#include <QList>
#include <QByteArray>
#include "joynr/Util.h"

«IF type.members.empty»
#include <tuple>
«ENDIF»

//include complex Datatype headers.
«FOR member: getRequiredIncludesFor(type)»
	#include "«member»"
«ENDFOR»

#include "«stdTypeUtil.getIncludeOf(type)»"

#ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4373 )
#endif

// Disable compiler warnings.
#pragma GCC diagnostic ignored "-Wunused-function"

«getNamespaceStarter(type)»

/**
«appendDoxygenSummaryAndWriteSeeAndDescription(type, " *")»
 */
class «getDllExportMacro()» «typeName» : public «IF hasExtendsDeclaration(type)»«getExtendedType(type).joynrNameQt»«ELSE»QObject«ENDIF»{
	Q_OBJECT

	«FOR member: getMembers(type)»
		«val membername = member.joynrName»
		«IF isArray(member)»
			/**
			 * @brief «appendDoxygenComment(member, "* ")»
			 */
			Q_PROPERTY(QList<QVariant> «membername» READ get«membername.toFirstUpper»Internal WRITE set«membername.toFirstUpper»Internal)
		«ELSEIF isByteBuffer(member.type)»
			/**
			 * @brief «appendDoxygenComment(member, "* ")»
			 */
			Q_PROPERTY(QByteArray «membername» READ get«membername.toFirstUpper»Internal WRITE set«membername.toFirstUpper»Internal)
		«ELSE»
			«IF isEnum(member.type)»
				/**
				 * @brief «appendDoxygenComment(member, "* ")»
				 */
				Q_PROPERTY(QString «membername» READ get«membername.toFirstUpper»Internal WRITE set«membername.toFirstUpper»Internal)
			«ELSE»
				// https://bugreports.qt-project.org/browse/QTBUG-2151 for why this replace is necessary
				/**
				 * @brief «appendDoxygenComment(member, "* ")»
				 */
				Q_PROPERTY(«member.typeName.replace("::","__")» «membername» READ get«membername.toFirstUpper» WRITE set«membername.toFirstUpper»)
				Q_PROPERTY(«member.typeName.replace("::","__")» «membername» READ get«membername.toFirstUpper» WRITE set«membername.toFirstUpper»)
			«ENDIF»
		«ENDIF»
	«ENDFOR»

public:
	//general methods
	/** @brief Constructor */
	«typeName»();
	«IF !getMembersRecursive(type).empty»
	/**
	 * @brief Parameterized constructor
	 «FOR member: getMembersRecursive(type)»
	 «appendDoxygenParameter(member, "*")»
	 «ENDFOR»
	 */
	«typeName»(
		«FOR member: getMembersRecursive(type) SEPARATOR","»
			«member.typeName» «member.joynrName»
		«ENDFOR»
	);
	«ENDIF»

	/** @brief Copy constructor */
	«typeName»(const «typeName»& «typeName.toFirstLower»Obj);

    /** @brief Destructor */
	virtual ~«typeName»();

    /**
	 * @brief Stringifies the class
	 * @return stringified class content
	 */
	virtual QString toString() const;

    /**
	 * @brief assigns an object
	 * @return reference to the object assigned to
	 */
	«typeName»& operator=(const «typeName»& «typeName.toFirstLower»Obj);

    /**
	 * @brief equality operator
	 * @param «typeName.toFirstLower»Obj reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	virtual bool operator==(const «typeName»& «typeName.toFirstLower»Obj) const;

    /**
	 * @brief unequality operator
	 * @param «typeName.toFirstLower»Obj reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	virtual bool operator!=(const «typeName»& «typeName.toFirstLower»Obj) const;

    /**
	 * @brief calculate hashCode for the object
	 * @return the calculated hashCode
	 */
	virtual uint hashCode() const;

	//getters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		«IF isArray(member)»
			/**
			 * @brief Gets «joynrName.toFirstUpper»
			 * @return «appendDoxygenComment(member, "* ")»
			 */
			QList<QVariant> get«joynrName.toFirstUpper»Internal() const;
		«ELSEIF isByteBuffer(member.type)»
			/**
			 * @brief Gets «joynrName.toFirstUpper»
			 * @return «appendDoxygenComment(member, "* ")»
			 */
			QByteArray get«joynrName.toFirstUpper»Internal() const;
		«ELSE»
			«IF isEnum(member.type)»
				/**
				 * @brief Gets «joynrName.toFirstUpper»
				 * @return «appendDoxygenComment(member, "* ")»
				 */
				QString get«joynrName.toFirstUpper»Internal() const;
			 «ENDIF»
		«ENDIF»
		/**
		 * @brief: Gets «joynrName.toFirstUpper»
		 * @return «appendDoxygenComment(member, "* ")»
		 */
		«member.typeName» get«joynrName.toFirstUpper»() const;
	«ENDFOR»

	//setters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		«IF isArray(member)»
			/**
			 * @brief Sets «joynrName.toFirstUpper»
			 «appendDoxygenParameter(member, "*")»
			 */
			void set«joynrName.toFirstUpper»Internal(const QList<QVariant>& «joynrName»);
		«ELSEIF isByteBuffer(member.type)»
			/**
			 * @brief Sets «joynrName.toFirstUpper»
			 «appendDoxygenParameter(member, "*")»
			 */
			void set«joynrName.toFirstUpper»Internal(const QByteArray& «joynrName»);
		«ELSE»
			«IF isEnum(member.type)»
				/**
				 * @brief Sets «joynrName.toFirstUpper»
				 «appendDoxygenParameter(member, "*")»
				 */
				void set«joynrName.toFirstUpper»Internal(const QString& «joynrName»);
			 «ENDIF»
		«ENDIF»
		/**
		 * @brief Sets «joynrName.toFirstUpper»
		 «appendDoxygenParameter(member, "*")»
		 */
		void set«joynrName.toFirstUpper»(const «member.typeName»& «joynrName»);
	«ENDFOR»
	
	//copy methods for Qt extraction
	«IF !type.members.empty»
		/**
		 * @brief convert QT specific type value to standard type value
		 * @param from variable of QT specific type «typeName» whose contents should be converted
		 * @param to variable to hold converted value of std type «stdTypeUtil.getTypeName(type)»
		 */
		static void createStd(const «typeName»& from, «stdTypeUtil.getTypeName(type)»& to);

		/**
		 * @brief convert standard C++ type value to QT specific type value
		 * @param from variable of std type «stdTypeUtil.getTypeName(type)» whose contents should be converted
		 * @param to variable to hold converted value of QT specific type «typeName»
		 */
		static void createQt(const «stdTypeUtil.getTypeName(type)»& from, «typeName»& to);

	«ENDIF»
	/**
	 * @brief convert QT specific type value to standard type value
	 * @param from variable of QT specific type «typeName» whose contents should be converted
	 * @return converted value of std type «stdTypeUtil.getTypeName(type)»
	 */
	static «stdTypeUtil.getTypeName(type)» createStd(const «typeName»& from);

	/**
	 * @brief convert standard C++ type value to QT specific type value
	 * @param from variable of std type «stdTypeUtil.getTypeName(type)» whose contents should be converted
	 * @return converted value of QT specific type «typeName»
	 */
	static «typeName» createQt(const «stdTypeUtil.getTypeName(type)»& from);

private:
	//members
	«FOR member: getMembers(type)»
		 «member.typeName» m_«member.joynrName»;
	«ENDFOR»
	void registerMetatypes();
};

«getNamespaceEnder(type)»

namespace joynr {
template <>
inline QList<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»> Util::valueOf<QList<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»>>(const QVariant& variant){
   return «joynrGenerationPrefix»::Util::convertVariantListToList<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»>(variant.value<QVariantList>());
}
}
«««		https://bugreports.qt-project.org/browse/QTBUG-2151 for why this typedef is necessary
typedef «getPackagePathWithJoynrPrefix(type, "::")»::«typeName» «getPackagePathWithJoynrPrefix(type, "__")»__«typeName»;
Q_DECLARE_METATYPE(«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»)
Q_DECLARE_METATYPE(QList<«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»>)

inline uint qHash(const «getPackagePathWithJoynrPrefix(type, "::")»::«typeName»& key) {
	return key.hashCode();
}


#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif // «headerGuard»
'''
}
