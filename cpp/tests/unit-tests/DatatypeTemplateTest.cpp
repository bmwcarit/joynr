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
#include <gtest/gtest.h>
using namespace ::testing;
#include "joynr/types/Localisation_QtTrip.h"
#include "joynr/types/Localisation_QtGpsLocation.h"
#include "joynr/types/TestTypes_QtWord.h"
#include "joynr/types/TestTypes_QtVowel.h"

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

    types::Localisation::QtGpsLocation loc1 = types::Localisation::QtGpsLocation(1.1, 1.2, 1.3, types::Localisation::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::Localisation::QtGpsLocation loc2 = types::Localisation::QtGpsLocation(2.1, 2.2, 2.3, types::Localisation::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::Localisation::QtGpsLocation> inputList = QList<types::Localisation::QtGpsLocation>();
    QList<types::Localisation::QtGpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::Localisation::QtTrip trip = types::Localisation::QtTrip(inputList, "myfirstTryp");
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}


TEST(DatatypeTemplateTest, enterListOfGpsLocationsUsingSetter) {

    types::Localisation::QtGpsLocation loc1 = types::Localisation::QtGpsLocation(1.1, 1.2, 1.3, types::Localisation::QtGpsFixEnum::MODE2D, 1.4, 1.5, 1.6, 1.7, 18, 19, 110);
    types::Localisation::QtGpsLocation loc2 = types::Localisation::QtGpsLocation(2.1, 2.2, 2.3, types::Localisation::QtGpsFixEnum::MODE2D, 2.4, 2.5, 2.6, 2.7, 28, 29, 210);
    QList<types::Localisation::QtGpsLocation> inputList = QList<types::Localisation::QtGpsLocation>();
    QList<types::Localisation::QtGpsLocation> returnList;
    inputList.append(loc1);
    inputList.append(loc2);
    types::Localisation::QtTrip trip = types::Localisation::QtTrip();
    trip.setLocations(inputList);
    returnList = trip.getLocations();
    EXPECT_EQ( returnList, inputList);
    EXPECT_EQ( returnList.value(1), loc2);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumList) {

    QList<types::TestTypes::QtVowel::Enum> inputList = QList<types::TestTypes::QtVowel::Enum>();
    inputList.append(types::TestTypes::QtVowel::A);
    inputList.append(types::TestTypes::QtVowel::E);
    inputList.append(types::TestTypes::QtVowel::E);
    inputList.append(types::TestTypes::QtVowel::U);
    types::TestTypes::QtWord myword = types::TestTypes::QtWord(inputList);
    EXPECT_EQ( myword.getVowels().value(1), types::TestTypes::QtVowel::E);
}

TEST(DatatypeTemplateTest, enterAndRetrieveEnumListviaInternalSetter) {

    QList<QVariant> inputSerializedList = QList<QVariant>();
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("A")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("E")));
    inputSerializedList.append(QVariant::fromValue(QStringLiteral("U")));
    types::TestTypes::QtWord myWord = types::TestTypes::QtWord();
    myWord.setVowelsInternal(inputSerializedList);
    EXPECT_EQ( myWord.getVowels().value(1), types::TestTypes::QtVowel::E);

    QList<types::TestTypes::QtVowel::Enum> inputList = QList<types::TestTypes::QtVowel::Enum>();
    inputList.append(types::TestTypes::QtVowel::A);
    inputList.append(types::TestTypes::QtVowel::E);
    inputList.append(types::TestTypes::QtVowel::E);
    inputList.append(types::TestTypes::QtVowel::U);
    types::TestTypes::QtWord myVowelWord =types::TestTypes::QtWord(inputList);

    EXPECT_EQ( myVowelWord, myWord);
}
