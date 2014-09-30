/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
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
 * #L%
 */
#ifndef UTIL_H_
#define UTIL_H_

#include "joynr/JoynrCommonExport.h"
#include "joynr/joynrlogging.h"

#include <QByteArray>
#include <QList>
#include <QMetaEnum>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>


namespace joynr {

/**
  * \class Util
  * \brief Container class for helper methods
  */
class JOYNRCOMMON_EXPORT Util {
public:
    /**
      * Splits a byte array representation of multiple JSON objects into
      * a list of byte arrays, each containing a single JSON object.
      */
    static QList<QByteArray> splitIntoJsonObjects(const QByteArray& jsonStream);

    /**
      * Converts an enum value to the corresponding name of the value.
      * \param T, the qt class that defines the enum, e.g. QProcess
      * \param enumName, the name of the enum, e.g. ProcessError (defined in QProcess)
      * \param value, the int value to convert
      */
    template<class T>
    static QString convertEnumValueToString(const char* enumName, int value) {
        QMetaEnum metaEnum = T::staticMetaObject.enumerator(
                    T::staticMetaObject.indexOfEnumerator(enumName)
        );
        return QLatin1String(metaEnum.valueToKey(value));
    }

    static QString attributeGetterFromName(const QString& attributeName);

    template <class T>
    static typename T::Enum convertVariantToEnum(const QVariant& v) {
        QMetaEnum metaEnum = T::staticMetaObject.enumerator(0);
        return static_cast<typename T::Enum>(
                    metaEnum.keyToValue(v.toString().toLatin1().data()));
    }

    template <class T>
    static QList<typename T::Enum> convertVariantListToEnumList(const QVariantList& variantList) {
        QList<typename T::Enum> ret;
        ret.reserve(variantList.length());
        foreach ( const QVariant& v, variantList ) {
            ret.append(convertVariantToEnum<T>(v));
        }
        return ret;
    }

    template <class T>
    static QList<QVariant> convertListToVariantList(const QList<T>& inputList){
        QList<QVariant> ret;
        ret.reserve(inputList.length());
        foreach (const T& q, inputList) {
            ret.append(QVariant::fromValue(q));
        }
        return ret;
    }

    template <class T>
    static QList<T> convertVariantListToList(const QList<QVariant>& inputList){
        QList<T> ret;
        ret.reserve(inputList.length());
        foreach( const QVariant& q, inputList ) {
            assert(q.canConvert<T>());
            ret.append(q.value<T>());
        }
        return ret;
    }

    template <class T>
    static QList<T> convertIntListToEnumList(const QList<int>& inputList){
        QList<T> ret;
        ret.reserve(inputList.length());
        foreach(const int& i, inputList ) {
            ret.append((T)i);
        }
        return ret;
    }

    /**
     * Create a Uuid for use in Joynr.
     *
     * The QUuid class produces UUIDs surrounded by curly braces and this
     * function removes them.
     */
    static QString createUuid();

    /**
     * Remove the leading namespace from a class name.
     *
     * e.g joynr::Reply gets converted to Reply
     *     joynr::tests::Base gets converted to tests::Base
     */
    static QString removeNamespace(const QString& className);

    /**
     * Log a serialized Joynr message
     */
    static void logSerializedMessage(joynr_logging::Logger *logger,
                                     const QString& explanation,
                                     const QString &message);

    template<typename... Ts>
    static int getTypeId();

    template<int TupleSize>
    struct ExpandTupleIntoFunctionArguments {
        template<typename Function, typename FunctionClass, typename Tuple, typename... Arguments>
        static inline auto expandTupleIntoFunctionArguments(
                Function& func,
                FunctionClass& funcClass,
                Tuple& tuple,
                Arguments&... args)
        -> decltype(ExpandTupleIntoFunctionArguments<TupleSize-1>::
                    expandTupleIntoFunctionArguments(
                        func, funcClass, tuple, std::get<TupleSize-1>(tuple), args...)){

            return ExpandTupleIntoFunctionArguments<TupleSize-1>::
                    expandTupleIntoFunctionArguments(
                        func, funcClass, tuple, std::get<TupleSize-1>(tuple), args...);
        }
    };

    template<typename Function, typename FunctionClass, typename Tuple>
    static inline auto expandTupleIntoFunctionArguments(
            Function& func,
            FunctionClass& funcClass,
            Tuple& tuple)
    -> decltype(ExpandTupleIntoFunctionArguments<std::tuple_size<
                typename std::decay<Tuple>::type>::value>::
                expandTupleIntoFunctionArguments(func, funcClass, tuple)){

        return ExpandTupleIntoFunctionArguments<std::tuple_size<
                typename std::decay<Tuple>::type>::value>::
                expandTupleIntoFunctionArguments(func, funcClass, tuple);
    }

private:
    static joynr_logging::Logger* logger;

    template<typename T, typename... Ts>
    static int getTypeId_split() {
        int prime = 31;
        return qMetaTypeId<T>() + prime*getTypeId<Ts...>();
    }
};

template<typename... Ts> inline
int Util::getTypeId() {
    return getTypeId_split<Ts...>();
}

template<> inline
int Util::getTypeId<>() {
    return 0;
}

template<>
struct Util::ExpandTupleIntoFunctionArguments<0> {
    template<typename Function, typename FunctionClass, typename Tuple, typename... Arguments>
    static inline auto expandTupleIntoFunctionArguments(
            Function& func,
            FunctionClass& funcClass,
            Tuple& tuple,
            Arguments&... args)
    -> decltype(func(funcClass, args...)){

        return func(funcClass, args...);
    }
};



} // namespace joynr
#endif /* UTIL_H_ */
