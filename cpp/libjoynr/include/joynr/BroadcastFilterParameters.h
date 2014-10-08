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
#ifndef BROADCASTFILTERPARAMETERS_H
#define BROADCASTFILTERPARAMETERS_H

#include <QMap>
#include <QString>
#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <QMetaType>

#include "joynr/JoynrCommonExport.h"

namespace joynr {

/**
 * \class BroadcastFilterParameters
 * \brief The BroadcastFilterParameters class represents generic filter parameters
 * for selective broadcasts
 */
class JOYNRCOMMON_EXPORT BroadcastFilterParameters : public QObject {
    Q_OBJECT

    Q_PROPERTY(QMap filterParameters READ getFilterParameters WRITE setFilterParameters)

public:
    BroadcastFilterParameters();
    BroadcastFilterParameters(const BroadcastFilterParameters& filterParameters);
    BroadcastFilterParameters& operator=(const BroadcastFilterParameters& filterParameters);
    ~BroadcastFilterParameters();
    bool operator==(const BroadcastFilterParameters& filterParameters) const;

    void setFilterParameter(QString parameter, QString value);
    void setFilterParameters(const QMap<QString, QVariant> &value);

    virtual bool equals(const QObject& other) const;

    QMap<QString, QVariant> getFilterParameters() const;
    QString getFilterParameter(QString parameter) const;

private:
    QMap<QString, QVariant> filterParameters;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::BroadcastFilterParameters)
Q_DECLARE_METATYPE(QSharedPointer<joynr::BroadcastFilterParameters>)
#endif // BROADCASTFILTERPARAMETERS_H
