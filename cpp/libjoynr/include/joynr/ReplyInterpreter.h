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
#ifndef REPLYINTERPRETER_H
#define REPLYINTERPRETER_H

#include "joynr/IReplyInterpreter.h"
#include "joynr/ReplyCaller.h"
#include "joynr/Reply.h"
#include "joynr/joynrlogging.h"
#include "joynr/JsonSerializer.h"
#include "joynr/Util.h"

namespace joynr {

template <class T>
class ReplyInterpreter :public IReplyInterpreter{
public:
    ReplyInterpreter() {}

    void execute (QSharedPointer<IReplyCaller> caller, const Reply& reply){
        assert (!caller.isNull());

        T value = reply.getResponse().value<T>();
        QSharedPointer< ReplyCaller<T> > typedCallerQsp = caller.dynamicCast<ReplyCaller<T> >();

        // value is copied in onSuccess
        LOG_TRACE(logger, "Reply received: notifying return value received");
        typedCallerQsp->returnValue(value);
    }
private:
    static joynr_logging::Logger* logger;
};

//specialisation for Lists.. if the value is of Type QList<T> it has to be converted from
//QList<QVariant> to QList<T> before being passed to the ReplyCaller.
template <class T>
class ReplyInterpreter<QList<T> > :public IReplyInterpreter{
public:
    ReplyInterpreter() {}

    void execute (QSharedPointer<IReplyCaller> caller, const Reply& reply){
        assert (!caller.isNull());

        QList<QVariant> qvList = reply.getResponse().value<QList<QVariant> >();
        QSharedPointer<ReplyCaller<QList<T> > > typedCallerQsp = caller.dynamicCast<ReplyCaller<QList<T> > >();
        QList<T> intList = Util::convertVariantListToList<T>(qvList);
        // value is copied in onSuccess
        typedCallerQsp->returnValue(intList);
    }
private:
};


template <class T>
joynr_logging::Logger* ReplyInterpreter<T>::logger = joynr_logging::Logging::getInstance()->getLogger("MSG", "ReplyInterpreter");

template <>
class ReplyInterpreter <void> :public IReplyInterpreter{
public:
    ReplyInterpreter() {
        qRegisterMetaType<Reply>();
    }

    void execute (QSharedPointer<IReplyCaller> caller, const Reply& reply){
        assert (!caller.isNull());
        Q_UNUSED(reply);//the reply should be empty, and is just passed in to match the common interface
        QSharedPointer< ReplyCaller<void> > typedCallerQsp = caller.dynamicCast<ReplyCaller<void> >();
        typedCallerQsp->returnValue();
    }
};

/**
  * Class that handles conversion of enum return values
  * Template parameter T is the Enum wrapper class
  */
template <class T>
class EnumReplyInterpreter : public IReplyInterpreter{
public:
    EnumReplyInterpreter() {}

    void execute (QSharedPointer<IReplyCaller> caller, const Reply& reply){
        assert (!caller.isNull());

        typename T::Enum value = Util::convertVariantToEnum<T>(reply.getResponse());
        QSharedPointer< ReplyCaller<typename T::Enum> > typedCallerQsp = caller.dynamicCast<ReplyCaller<typename T::Enum> >();

        // value is copied in onSuccess
        typedCallerQsp->returnValue(value);
    }
};

template <class T>
class EnumReplyInterpreter<QList<T> > :public IReplyInterpreter{
public:
    EnumReplyInterpreter() {}

    void execute (QSharedPointer<IReplyCaller> caller, const Reply& reply){
        assert (!caller.isNull());

        QList<QVariant> qvList = reply.getResponse().value<QList<QVariant> >();
        QSharedPointer<ReplyCaller<QList<typename T::Enum> > > typedCallerQsp = caller.dynamicCast<ReplyCaller<QList<typename T::Enum> > >();
        QList<typename T::Enum> enumList = Util::convertVariantListToEnumList<T>(qvList);
        // value is copied in onSuccess
        typedCallerQsp->returnValue(enumList);
    }
};

} // namespace joynr
#endif // REPLYINTERPRETER_H
