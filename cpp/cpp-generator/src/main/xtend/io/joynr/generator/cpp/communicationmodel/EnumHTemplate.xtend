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

import com.google.inject.Inject
import org.franca.core.franca.FEnumerationType
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.util.EnumTemplate

class EnumHTemplate implements EnumTemplate{

	@Inject
	private extension TemplateBase

	@Inject
	private extension JoynrCppGeneratorExtensions

	override generate(FEnumerationType type)
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_ENUM_"+getPackagePathWithJoynrPrefix(type, "_")+"_"+typeName+"_h").toUpperCase»
«warning»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»
#include <QObject>
#include <QMetaType>
#include "joynr/Util.h"

«getNamespaceStarter(type)»

class «getDllExportMacro()» «typeName» : public QObject {
	Q_OBJECT
	Q_ENUMS(«getNestedEnumName()»)
public:
	enum «getNestedEnumName()» {
		«FOR enumtype : getEnumElements(type) SEPARATOR ','»
			«enumtype.joynrName»
		«ENDFOR»
	};
	// Constructors required by QT metatype system
	«typeName»() : QObject() {}
	«typeName»(const «typeName»& o) : QObject() { Q_UNUSED(o); }
};

«getNamespaceEnder(type)»

namespace joynr {
template <>
inline «getPackagePathWithJoynrPrefix(type, "::")»::«typeName»::«getNestedEnumName()» joynr::Util::valueOf<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»::«getNestedEnumName()»>(const QVariant& variant)
{
  return «joynrGenerationPrefix»::Util::convertVariantToEnum<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»>(variant);
}

template <>
inline QList<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»::Enum> joynr::Util::valueOf<QList<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»::Enum>>(const QVariant& variant){
   return «joynrGenerationPrefix»::Util::convertVariantListToEnumList<«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»>(variant.value<QVariantList>());
}
}
// Metatype for the wrapper class
typedef «getPackagePathWithJoynrPrefix(type, "::")»::«typeName» «getPackagePathWithJoynrPrefix(type, "__")»__«typeName»;
Q_DECLARE_METATYPE(«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»)

// Metatypes for the «getNestedEnumName()»
typedef «getPackagePathWithJoynrPrefix(type, "::")»::«typeName»::«getNestedEnumName()» «getPackagePathWithJoynrPrefix(type, "__")»__«typeName»__«getNestedEnumName()»;
Q_DECLARE_METATYPE(«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»__«getNestedEnumName()»)
Q_DECLARE_METATYPE(QList<«getPackagePathWithJoynrPrefix(type, "__")»__«typeName»__«getNestedEnumName()»>)

inline uint qHash(«getPackagePathWithJoynrPrefix(type, "::")»::«typeName»::«getNestedEnumName()» key, uint seed = 0) {
	return uint(key) ^ seed;
}

#endif // «headerGuard»
'''
}