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
#include "joynr/types/localisation/QtTrip.h"
#include "joynr/types/localisation/QtGpsLocation.h"
#include "joynr/types/testtypes/QtWord.h"
#include "joynr/types/testtypes/QtVowel.h"

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

    types::localisation::QtGpsLocation loc1 = types::localisation::QtGpsLocation(1.1, 1.2, 1.3, types::localisation::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::localisation::QtGpsLocation loc2 = types::localisation::QtGpsLocation(2.1, 2.2, 2.3, types::localisation::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::localisation::QtGpsLocation> inputList = QList<types::localisation::QtGpsLocation>();
    QList<types::localisation::QtGpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::localisation::QtTrip trip = types::localisation::QtTrip(inputList, "myfirstTryp");
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}


TEST(DatatypeTemplateTest, enterListOfGpsLocationsUsingSetter) {

    types::localisation::QtGpsLocation loc1 = types::localisation::QtGpsLocation(1.1, 1.2, 1.3, types::localisation::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::localisation::QtGpsLocation loc2 = types::localisation::QtGpsLocation(2.1, 2.2, 2.3, types::localisation::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::localisation::QtGpsLocation> inputList = QList<types::localisation::QtGpsLocation>();
    QList<types::localisation::QtGpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::localisation::QtTrip trip = types::localisation::QtTrip();
    trip.setLocations(inputList);
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumList) {

    QList<types::testtypes::QtVowel::Enum> inputList = QList<types::testtypes::QtVowel::Enum>();
    inputList.append(types::testtypes::QtVowel::A);
    inputList.append(types::testtypes::QtVowel::E);
    inputList.append(types::testtypes::QtVowel::E);
    inputList.append(types::testtypes::QtVowel::U);
    types::testtypes::QtWord myword = types::testtypes::QtWord(inputList);
    EXPECT_EQ( myword.getVowels().value(1), types::testtypes::QtVowel::E);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumListviaInternalSetter) {

    QList<QVariant> inputSerializedList = QList<QVariant>();
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("A")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("U")));
    types::testtypes::QtWord myWord = types::testtypes::QtWord();
    myWord.setVowelsInternal(inputSerializedList);
    EXPECT_EQ( myWord.getVowels().value(1), types::testtypes::QtVowel::E);

    QList<types::testtypes::QtVowel::Enum> inputList = QList<types::testtypes::QtVowel::Enum>();
    inputList.append(types::testtypes::QtVowel::A);
    inputList.append(types::testtypes::QtVowel::E);
    inputList.append(types::testtypes::QtVowel::E);
    inputList.append(types::testtypes::QtVowel::U);
    types::testtypes::QtWord myVowelWord =types::testtypes::QtWord(inputList);

    EXPECT_EQ( myVowelWord, myWord);
}
