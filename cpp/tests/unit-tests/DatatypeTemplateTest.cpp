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
#include "gtest/gtest.h"
using namespace ::testing;
#include "joynr/types/QtTrip.h"
#include "joynr/types/QtGpsLocation.h"
#include "joynr/types/QtWord.h"
#include "joynr/types/QtVowel.h"

using namespace joynr;

//    Those test-enums and structs must be part of IDL for this test to compile:

//    struct QtTrip {
//      tripId : string
//      locations : Location[]
//      tripTitle : string
//    }
//    enum QtVowel {
//         a, e, i, o, u
//    }
//    struct QtWord {
//        vowels : QtVowel[]
//    }

TEST(DatatypeTemplateTest, enterAndRetrieveListOfGpsLocationsFromTrip) {

    types::QtGpsLocation loc1 = types::QtGpsLocation(1.1, 1.2, 1.3, types::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::QtGpsLocation loc2 = types::QtGpsLocation(2.1, 2.2, 2.3, types::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::QtGpsLocation> inputList = QList<types::QtGpsLocation>();
    QList<types::QtGpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::QtTrip trip = types::QtTrip(inputList, "myfirstTryp");
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}


TEST(DatatypeTemplateTest, enterListOfGpsLocationsUsingSetter) {

    types::QtGpsLocation loc1 = types::QtGpsLocation(1.1, 1.2, 1.3, types::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::QtGpsLocation loc2 = types::QtGpsLocation(2.1, 2.2, 2.3, types::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::QtGpsLocation> inputList = QList<types::QtGpsLocation>();
    QList<types::QtGpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::QtTrip trip = types::QtTrip();
    trip.setLocations(inputList);
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumList) {

    QList<types::QtVowel::Enum> inputList = QList<types::QtVowel::Enum>();
    inputList.append(types::QtVowel::A);
    inputList.append(types::QtVowel::E);
    inputList.append(types::QtVowel::E);
    inputList.append(types::QtVowel::U);
    types::QtWord myword = types::QtWord(inputList);
    EXPECT_EQ( myword.getVowels().value(1), types::QtVowel::E);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumListviaInternalSetter) {

    QList<QVariant> inputSerializedList = QList<QVariant>();
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("A")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("U")));
    types::QtWord myWord = types::QtWord();
    myWord.setVowelsInternal(inputSerializedList);
    EXPECT_EQ( myWord.getVowels().value(1), types::QtVowel::E);

    QList<types::QtVowel::Enum> inputList = QList<types::QtVowel::Enum>();
    inputList.append(types::QtVowel::A);
    inputList.append(types::QtVowel::E);
    inputList.append(types::QtVowel::E);
    inputList.append(types::QtVowel::U);
    types::QtWord myVowelWord =types::QtWord(inputList);

    EXPECT_EQ( myVowelWord, myWord);
}
