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
#ifndef REPLY_H
#define REPLY_H

#include "joynr/JoynrCommonExport.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>
#include <QSharedPointer>

namespace joynr
{

class JOYNRCOMMON_EXPORT Reply : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString requestReplyId READ getRequestReplyId WRITE setRequestReplyId)
    Q_PROPERTY(QList<QVariant> response READ getResponse WRITE setResponse)
public:
    Reply& operator=(const Reply& other);
    bool operator==(const Reply& other) const;
    bool operator!=(const Reply& other) const;

    const static Reply NULL_RESPONSE;

    Reply(const Reply& other);
    Reply();

    QString getRequestReplyId() const;
    void setRequestReplyId(QString requestReplyId);

    QList<QVariant> getResponse() const;
    void setResponse(QList<QVariant> response);

private:
    QString requestReplyId;
    QList<QVariant> response;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::Reply)
Q_DECLARE_METATYPE(QSharedPointer<joynr::Reply>)
#endif // REPLY_H
