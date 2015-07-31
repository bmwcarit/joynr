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
#ifndef PRETTYPRINT_H_
#define PRETTYPRINT_H_

#include <gtest/gtest.h>
#include <QString>
#include <QChar>
#include <QByteArray>
#include <QtCore/QUrl>
#include <iostream>

#include "joynr/JsonSerializer.h"
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/types/QtDiscoveryEntry.h"

#define EXPECT_EQ_QSTRING(a, b) EXPECT_EQ(a, b) << "  Actual: " << b.toStdString() << std::endl << "Expected: " << a.toStdString() << std::endl
#define EXPECT_EQ_QBYTEARRAY(a, b) EXPECT_EQ(a, b) << "  Actual: " << b.constData() << std::endl << "Expected: " << a.constData() << std::endl

//void initPretty(void);

namespace joynr {
// NOTE: Choosing the right PrintTo method is done by template magic by
//       the compiler. Therefore, the point in time when the PrintTo method
//       is defined is crucial. So consider defining the method in the same
//       file where your type is defined.
//
// The following PrintTo's are defined directly in the file where the type is
// defined:
//    class JoynrMessage;
//    void PrintTo(const joynr::JoynrMessage& value, ::std::ostream* os);
//    class MessagingQos;
//    void PrintTo(const joynr::MessagingQos& value, ::std::ostream* os);
namespace types {
    class QtTStruct;
    void PrintTo(const joynr::types::QtTStruct& value, ::std::ostream* os);
    class QtGpsLocation;
    void PrintTo(const joynr::types::QtGpsLocation& value, ::std::ostream* os);
    class QtTrip;
    void PrintTo(const joynr::types::QtTrip& value, ::std::ostream* os);
}
namespace system {
    class QtDiscoveryEntry;
    void PrintTo(const joynr::types::QtDiscoveryEntry& value, ::std::ostream* os);
    class QtWebSocketAddress;
    void PrintTo(const joynr::system::QtWebSocketAddress& value, ::std::ostream* os);
}
}
void PrintTo(const QString& value, ::std::ostream* os);
void PrintTo(const QChar& value, ::std::ostream* os);
void PrintTo(const QByteArray& value, ::std::ostream* os);
void PrintTo(const QUrl& value, ::std::ostream* os);
//void PrintTo(const QObject& value, ::std::ostream* os);
//void PrintTo(const QVariant& value, ::std::ostream* os);
void PrintTo(const joynr::RequestStatusCode& value, ::std::ostream* os);
void PrintTo(const joynr::RequestStatus& value, ::std::ostream* os);

#endif /* PRETTYPRINT_H_ */
