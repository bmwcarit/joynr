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
#ifndef QTBROADCASTFILTERPARAMETERS_H
#define QTBROADCASTFILTERPARAMETERS_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <QObject>
#include <QMetaType>

#include "joynr/JoynrCommonExport.h"
#include "joynr/BroadcastFilterParameters.h"

#include <memory>

namespace joynr
{

/**
 * \class QtBroadcastFilterParameters
 * \brief The QtBroadcastFilterParameters class represents generic filter parameters
 * for selective broadcasts
 */
class JOYNRCOMMON_EXPORT QtBroadcastFilterParameters : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QMap filterParameters MEMBER filterParameters)

public:
    QtBroadcastFilterParameters();
    QtBroadcastFilterParameters(const QtBroadcastFilterParameters& filterParameters);
    QtBroadcastFilterParameters& operator=(const QtBroadcastFilterParameters& filterParameters);
    ~QtBroadcastFilterParameters();
    bool operator==(const QtBroadcastFilterParameters& filterParameters) const;

    void setFilterParameter(const QString& parameter, const QString& value);
    void setFilterParameters(const QMap<QString, QString>& value);

    virtual bool equals(const QObject& other) const;

    QMap<QString, QString> getFilterParameters() const;
    QString getFilterParameter(const QString& parameter) const;

    static QtBroadcastFilterParameters createQt(const BroadcastFilterParameters& from);
    static BroadcastFilterParameters createStd(const QtBroadcastFilterParameters& from);

private:
    QMap<QString, QVariant> filterParameters;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::QtBroadcastFilterParameters)
Q_DECLARE_METATYPE(std::shared_ptr<joynr::QtBroadcastFilterParameters>)
#endif // QTBROADCASTFILTERPARAMETERS_H
