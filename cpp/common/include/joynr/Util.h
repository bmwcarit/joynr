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

#include <QtGlobal>
#include <QByteArray>
#include <QList>
#include <QMetaEnum>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace joynr
{

/**
  * \class Util
  * \brief Container class for helper methods
  */
class JOYNRCOMMON_EXPORT Util
{
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
    template <class T>
    static QString convertEnumValueToString(const char* enumName, int value)
    {
        QMetaEnum metaEnum =
                T::staticMetaObject.enumerator(T::staticMetaObject.indexOfEnumerator(enumName));
        return QLatin1String(metaEnum.valueToKey(value));
    }

    /**
      * Converts an enum value to a QVariant for use by the serializer
      * \param T, the qt class that surrounds the enum
      * \param value, the enum value to convert
      */
    template <class T>
    static QVariant convertEnumToVariant(typename T::Enum value)
    {
        QMetaEnum metaEnum = T::staticMetaObject.enumerator(0);
        return QVariant(metaEnum.valueToKey(value));
    }

    static QString attributeGetterFromName(const QString& attributeName);

    template <class T>
    static typename T::Enum convertVariantToEnum(const QVariant& v)
    {
        QMetaEnum metaEnum = T::staticMetaObject.enumerator(0);
        return static_cast<typename T::Enum>(metaEnum.keyToValue(v.toString().toLatin1().data()));
    }

    template <class T>
    static QList<QVariant> convertEnumListToVariantList(const QList<typename T::Enum>& enumList)
    {
        QList<QVariant> variantList;
        variantList.reserve(enumList.length());
        foreach (const typename T::Enum e, enumList) {
            variantList.append(convertEnumToVariant<T>(e));
        }
        return variantList;
    }

    template <class T>
    static QList<typename T::Enum> convertVariantListToEnumList(const QList<QVariant>& variantList)
    {
        QList<typename T::Enum> enumList;
        enumList.reserve(variantList.length());
        foreach (const QVariant& v, variantList) {
            enumList.append(convertVariantToEnum<T>(v));
        }
        return enumList;
    }

    template <class T>
    static QList<QVariant> convertListToVariantList(const QList<T>& inputList)
    {
        QList<QVariant> ret;
        ret.reserve(inputList.length());
        foreach (const T& q, inputList) {
            ret.append(QVariant::fromValue(q));
        }
        return ret;
    }

    template <class T>
    static QList<T> convertVariantListToList(const QList<QVariant>& inputList)
    {
        QList<T> ret;
        ret.reserve(inputList.length());
        foreach (const QVariant& q, inputList) {
            assert(q.canConvert<T>());
            ret.append(q.value<T>());
        }
        return ret;
    }

    template <class T>
    static QList<T> convertIntListToEnumList(const QList<int>& inputList)
    {
        QList<T> ret;
        ret.reserve(inputList.length());
        foreach (const int& i, inputList) {
            ret.append((T)i);
        }
        return ret;
    }

    template <class T>
    static QList<int> convertEnumListToIntList(const QList<T>& enumList)
    {
        QList<int> enumAsIntList;
        enumAsIntList.reserve(enumList.length());
        foreach (const T& e, enumList) {
            enumAsIntList.append(e);
        }
        return enumAsIntList;
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
    static void logSerializedMessage(joynr_logging::Logger* logger,
                                     const QString& explanation,
                                     const QString& message);

    template <typename... Ts>
    static int getBroadcastTypeId();

    template <typename T>
    static T valueOf(const QVariant& variant);

    template <int TupleSize>
    struct ExpandTupleIntoFunctionArguments
    {
        template <typename Function, typename FunctionClass, typename Tuple, typename... Arguments>
        static inline auto expandTupleIntoFunctionArguments(Function& func,
                                                            FunctionClass& funcClass,
                                                            Tuple& tuple,
                                                            Arguments&... args)
                -> decltype(ExpandTupleIntoFunctionArguments<
                        TupleSize - 1>::expandTupleIntoFunctionArguments(func,
                                                                         funcClass,
                                                                         tuple,
                                                                         std::get<TupleSize - 1>(
                                                                                 tuple),
                                                                         args...))
        {

            return ExpandTupleIntoFunctionArguments<
                    TupleSize - 1>::expandTupleIntoFunctionArguments(func,
                                                                     funcClass,
                                                                     tuple,
                                                                     std::get<TupleSize - 1>(tuple),
                                                                     args...);
        }
    };

    template <typename Function, typename FunctionClass, typename Tuple>
    static inline auto expandTupleIntoFunctionArguments(Function& func,
                                                        FunctionClass& funcClass,
                                                        Tuple& tuple)
            -> decltype(ExpandTupleIntoFunctionArguments<std::tuple_size<typename std::decay<
                    Tuple>::type>::value>::expandTupleIntoFunctionArguments(func, funcClass, tuple))
    {

        return ExpandTupleIntoFunctionArguments<std::tuple_size<typename std::decay<
                Tuple>::type>::value>::expandTupleIntoFunctionArguments(func, funcClass, tuple);
    }

    template <typename... Ts>
    static std::tuple<Ts...> toValueTuple(QList<QVariant> list);

private:
    static joynr_logging::Logger* logger;

    template <typename T, typename... Ts>
    static int getTypeId_split()
    {
        int prime = 31;
        return prime * qMetaTypeId<T>() + prime * getBroadcastTypeId<Ts...>();
    }

    template <typename T, typename... Ts>
    static std::tuple<T, Ts...> toValueTuple_split(QList<QVariant> list)
    {
        T value = valueOf<T>(list.first());
        list.removeFirst();

        return std::tuple_cat(std::make_tuple(value), toValueTuple<Ts...>(list));
    }
};

template <typename T>
inline T Util::valueOf(const QVariant& variant)
{
    return variant.value<T>();
}

template <typename... Ts>
inline int Util::getBroadcastTypeId()
{
    return getTypeId_split<Ts...>();
}

template <>
inline int Util::getBroadcastTypeId<>()
{
    return 0;
}

template <>
struct Util::ExpandTupleIntoFunctionArguments<0>
{
    template <typename Function, typename FunctionClass, typename Tuple, typename... Arguments>
    static inline auto expandTupleIntoFunctionArguments(Function& func,
                                                        FunctionClass& funcClass,
                                                        Tuple& tuple,
                                                        Arguments&... args)
            -> decltype(func(funcClass, args...))
    {
        Q_UNUSED(tuple);
        return func(funcClass, args...);
    }
};

template <typename... Ts>
inline std::tuple<Ts...> Util::toValueTuple(QList<QVariant> list)
{
    return toValueTuple_split<Ts...>(list);
}

template <>
inline std::tuple<> Util::toValueTuple<>(QList<QVariant> list)
{
    assert(list.empty());
    return std::make_tuple();
}

} // namespace joynr
#endif /* UTIL_H_ */
