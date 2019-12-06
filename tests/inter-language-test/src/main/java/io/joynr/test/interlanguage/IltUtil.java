/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.test.interlanguage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.EnumerationWithoutDefinedValues;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.StructOfPrimitives;

public class IltUtil {
    private static final Logger LOG = LoggerFactory.getLogger(IltUtil.class);

    private static boolean useRestricted64BitRange = true;

    public static void setRestricted64BitRange(boolean value) {
        useRestricted64BitRange = value;
    }

    public static boolean getUseRestricted64BitRange() {
        return useRestricted64BitRange;
    }

    // List of Byte
    public static Byte[] fillByteArray(Byte[] byteArray) {
        byteArray[0] = (byte) 1;
        byteArray[1] = (byte) 127;
        if (!checkByteArray(byteArray)) {
            throw new RuntimeException("Internal error in fillByteArray");
        }
        return byteArray;
    }

    public static Byte[] createByteArray() {
        Byte[] byteArray = new Byte[2];
        fillByteArray(byteArray);
        return byteArray;
    }

    public static boolean checkByteArray(Byte[] byteArray) {
        if (byteArray.length != 2) {
            return false;
        }

        if (byteArray[0] != (byte) 1 || byteArray[1] != (byte) 127) {
            return false;
        }
        return true;
    }

    // List of uInt64
    public static Long[] fillUInt64Array(Long[] uInt64Array) {
        uInt64Array[0] = 1L;
        uInt64Array[1] = 127L;
        if (!checkUInt64Array(uInt64Array)) {
            throw new RuntimeException("Internal error in fillUInt64Array");
        }
        return uInt64Array;
    }

    public static Long[] createUInt64Array() {
        Long[] uInt64Array = new Long[2];
        fillUInt64Array(uInt64Array);
        return uInt64Array;
    }

    public static boolean checkUInt64Array(Long[] uInt64Array) {
        if (uInt64Array.length != 2) {
            return false;
        }

        if (uInt64Array[0] != 1L || uInt64Array[1] != 127L) {
            return false;
        }
        return true;
    }

    // List of Double
    public static Double[] fillDoubleArray(Double[] doubleArray) {
        doubleArray[0] = 1.1;
        doubleArray[1] = 2.2;
        doubleArray[2] = 3.3;
        if (!checkDoubleArray(doubleArray)) {
            throw new RuntimeException("Internal error in fillDoubleArray");
        }
        return doubleArray;
    }

    public static Double[] createDoubleArray() {
        Double[] doubleArray = new Double[3];
        fillDoubleArray(doubleArray);
        return doubleArray;
    }

    public static boolean checkDoubleArray(Double[] doubleArray) {
        if (doubleArray.length != 3) {
            return false;
        }

        if (!cmpDouble(doubleArray[0], 1.1) || !cmpDouble(doubleArray[1], 2.2) || !cmpDouble(doubleArray[2], 3.3)) {
            return false;
        }
        return true;
    }

    // List of ExtendedInterfaceEnumerationInTypeCollection
    public static ExtendedInterfaceEnumerationInTypeCollection[] fillExtendedInterfaceEnumerationInTypeCollectionArray(ExtendedInterfaceEnumerationInTypeCollection[] extendedInterfaceEnumerationInTypeCollectionArray) {
        extendedInterfaceEnumerationInTypeCollectionArray[0] = ExtendedInterfaceEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_INTERFACE;
        extendedInterfaceEnumerationInTypeCollectionArray[1] = ExtendedInterfaceEnumerationInTypeCollection.ENUM_I1_VALUE_3;
        if (!checkExtendedInterfaceEnumerationInTypeCollectionArray(extendedInterfaceEnumerationInTypeCollectionArray)) {
            throw new RuntimeException("Internal error in fillExtendedInterfaceEnumerationInTypeCollectionArray");
        }
        return extendedInterfaceEnumerationInTypeCollectionArray;
    }

    public static ExtendedInterfaceEnumerationInTypeCollection[] createExtendedInterfaceEnumerationInTypeCollectionArray() {
        ExtendedInterfaceEnumerationInTypeCollection[] extendedInterfaceEnumerationInTypeCollectionArray = new ExtendedInterfaceEnumerationInTypeCollection[2];
        fillExtendedInterfaceEnumerationInTypeCollectionArray(extendedInterfaceEnumerationInTypeCollectionArray);
        return extendedInterfaceEnumerationInTypeCollectionArray;
    }

    public static boolean checkExtendedInterfaceEnumerationInTypeCollectionArray(ExtendedInterfaceEnumerationInTypeCollection[] extendedInterfaceEnumerationInTypeCollectionArray) {
        if (extendedInterfaceEnumerationInTypeCollectionArray.length != 2) {
            return false;
        }

        if (extendedInterfaceEnumerationInTypeCollectionArray[0] != ExtendedInterfaceEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_INTERFACE
                || extendedInterfaceEnumerationInTypeCollectionArray[1] != ExtendedInterfaceEnumerationInTypeCollection.ENUM_I1_VALUE_3) {
            return false;
        }

        return true;
    }

    // List of ExtendedExtendedEnumeration
    public static ExtendedExtendedEnumeration[] fillExtendedExtendedEnumerationArray(ExtendedExtendedEnumeration[] extendedExtendedEnumerationArray) {
        extendedExtendedEnumerationArray[0] = ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_EXTENDED;
        extendedExtendedEnumerationArray[1] = ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        if (!checkExtendedExtendedEnumerationArray(extendedExtendedEnumerationArray)) {
            throw new RuntimeException("Internal error in fillExtendedExtendedEnumerationArray");
        }
        return extendedExtendedEnumerationArray;
    }

    public static ExtendedExtendedEnumeration[] createExtendedExtendedEnumerationArray() {
        ExtendedExtendedEnumeration[] extendedExtendedEnumerationArray = new ExtendedExtendedEnumeration[2];
        fillExtendedExtendedEnumerationArray(extendedExtendedEnumerationArray);
        return extendedExtendedEnumerationArray;
    }

    public static boolean checkExtendedExtendedEnumerationArray(ExtendedExtendedEnumeration[] extendedExtendedEnumerationArray) {
        if (extendedExtendedEnumerationArray.length != 2) {
            return false;
        }

        if (extendedExtendedEnumerationArray[0] != ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_EXTENDED
                || extendedExtendedEnumerationArray[1] != ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            return false;
        }

        return true;
    }

    // TYPE StructWithStringArray
    public static StructWithStringArray fillStructWithStringArray(StructWithStringArray structWithStringArray) {
        String[] stringArray = new String[2];
        stringArray[0] = "Hello";
        stringArray[1] = "World";
        // will override existing list in any case
        structWithStringArray.setStringArrayElement(stringArray);
        if (!checkStructWithStringArray(structWithStringArray)) {
            throw new RuntimeException("Internal error in fillStructWithStringArray");
        }
        return structWithStringArray;
    }

    public static StructWithStringArray createStructWithStringArray() {
        StructWithStringArray structWithStringArray = new StructWithStringArray();
        fillStructWithStringArray(structWithStringArray);
        return structWithStringArray;
    }

    public static boolean checkStructWithStringArray(StructWithStringArray structWithStringArray) {
        if (structWithStringArray == null) {
            return false;
        }
        String[] stringArray = structWithStringArray.getStringArrayElement();
        if (stringArray.length != 2) {
            return false;
        }

        if (!stringArray[0].equals("Hello") || !stringArray[1].equals("World")) {
            return false;
        }
        return true;
    }

    // TYPE List<StructWithStringArray>
    public static StructWithStringArray[] fillStructWithStringArrayArray(StructWithStringArray[] structWithStringArrayArray) {
        structWithStringArrayArray[0] = createStructWithStringArray();
        structWithStringArrayArray[1] = createStructWithStringArray();
        // will override existing list in any case
        if (!checkStructWithStringArrayArray(structWithStringArrayArray)) {
            throw new RuntimeException("Internal error in fillStructWithStringArrayArray");
        }
        return structWithStringArrayArray;
    }

    public static StructWithStringArray[] createStructWithStringArrayArray() {
        StructWithStringArray[] structWithStringArrayArray = new StructWithStringArray[2];
        fillStructWithStringArrayArray(structWithStringArrayArray);
        return structWithStringArrayArray;
    }

    public static boolean checkStructWithStringArrayArray(StructWithStringArray[] structWithStringArrayArray) {
        if (structWithStringArrayArray.length != 2) {
            return false;
        }
        if (!checkStructWithStringArray(structWithStringArrayArray[0])
                || !checkStructWithStringArray(structWithStringArrayArray[1])) {
            return false;
        }
        return true;
    }

    // TYPE BaseStructWithoutElements
    public static BaseStructWithoutElements createBaseStructWithoutElements() {
        BaseStructWithoutElements baseStructWithoutElements = new BaseStructWithoutElements();
        // nothing required, since this is an empty struct
        //
        // it was intended to be used for typecasts of some other struct
        // inheriting from this one. Not sure, if this really can work.
        return baseStructWithoutElements;
    }

    public static boolean checkBaseStructWithoutElements(BaseStructWithoutElements baseStructWithoutElements) {
        // nothing required, since this is an empty struct
        return true;
    }

    // TYPE BaseStruct
    public static BaseStruct fillBaseStruct(BaseStruct baseStruct) {
        baseStruct.setBaseStructString("Hiya");
        if (!(baseStruct.getBaseStructString().equals("Hiya"))) {
            throw new RuntimeException("Internal error in fillBaseStruct");
        }
        return baseStruct;
    }

    public static BaseStruct createBaseStruct() {
        BaseStruct baseStruct = new BaseStruct();
        fillBaseStruct(baseStruct);
        return baseStruct;
    }

    // TYPE ExtendedBaseStruct extends BaseStruct
    public static ExtendedBaseStruct fillExtendedBaseStruct(ExtendedBaseStruct extendedBaseStruct) {
        extendedBaseStruct.setEnumElement(Enumeration.ENUM_0_VALUE_3);
        fillBaseStruct(extendedBaseStruct);
        if (extendedBaseStruct.getEnumElement() != Enumeration.ENUM_0_VALUE_3
                || !extendedBaseStruct.getBaseStructString().equals("Hiya")) {
            throw new RuntimeException("Internal error in fillExtendedBaseStruct");
        }
        return extendedBaseStruct;
    }

    public static ExtendedBaseStruct createExtendedBaseStruct() {
        ExtendedBaseStruct extendedBaseStruct = new ExtendedBaseStruct();
        fillExtendedBaseStruct(extendedBaseStruct);
        return extendedBaseStruct;
    }

    // TYPE ExtendedBaseStruct extends ExtendedBaseStruct extends BaseStruct
    public static ExtendedExtendedBaseStruct fillExtendedExtendedBaseStruct(ExtendedExtendedBaseStruct extendedExtendedBaseStruct) {
        extendedExtendedBaseStruct.setEnumWithoutDefinedValuesElement(EnumerationWithoutDefinedValues.ENUM_0_VALUE_1);
        // joynr.interlanguagetest.Enumeration
        fillExtendedBaseStruct(extendedExtendedBaseStruct);
        if (extendedExtendedBaseStruct.getEnumWithoutDefinedValuesElement() != EnumerationWithoutDefinedValues.ENUM_0_VALUE_1
                || extendedExtendedBaseStruct.getEnumElement() != Enumeration.ENUM_0_VALUE_3
                || !extendedExtendedBaseStruct.getBaseStructString().equals("Hiya")) {
            throw new RuntimeException("Internal error in fillExtendedExtendedBaseStruct");
        }
        return extendedExtendedBaseStruct;
    }

    public static ExtendedExtendedBaseStruct createExtendedExtendedBaseStruct() {
        ExtendedExtendedBaseStruct extendedExtendedBaseStruct = new ExtendedExtendedBaseStruct();
        fillExtendedExtendedBaseStruct(extendedExtendedBaseStruct);
        return extendedExtendedBaseStruct;
    }

    // TYPE StructOfPrimitives
    public static StructOfPrimitives fillStructOfPrimitives(StructOfPrimitives structOfPrimitives) {
        structOfPrimitives.setBooleanElement(true);
        structOfPrimitives.setDoubleElement(1.1d);
        structOfPrimitives.setFloatElement(1.1f);
        structOfPrimitives.setInt8MinElement(Byte.MIN_VALUE);
        structOfPrimitives.setInt8MaxElement(Byte.MAX_VALUE);
        structOfPrimitives.setInt16MinElement(Short.MIN_VALUE);
        structOfPrimitives.setInt16MaxElement(Short.MAX_VALUE);
        structOfPrimitives.setInt32MinElement(Integer.MIN_VALUE);
        structOfPrimitives.setInt32MaxElement(Integer.MAX_VALUE);

        if (useRestricted64BitRange) {
            // Using reduced value range to stay compatible with
            // mastructOfPrimitivesimum safe integer range of Javascript
            structOfPrimitives.setInt64MinElement(-9007199254740991L);
            structOfPrimitives.setInt64MaxElement(9007199254740991L);
        } else {
            // Java and C++
            structOfPrimitives.setInt64MinElement(Long.MIN_VALUE);
            structOfPrimitives.setInt64MaxElement(Long.MAX_VALUE);
        }

        structOfPrimitives.setConstString("Hiya");

        // negative values are not allowed on the wire (JSON) for uint8
        // range
        structOfPrimitives.setUInt8MinElement((byte) 0);
        structOfPrimitives.setUInt8MaxElement((byte) 127);
        // structOfPrimitives.setUInt8MaxElement(new Short((short)255).byteValue());
        // structOfPrimitives.setUInt8MaxElement((byte)255);

        // negative values are not allowed on the wire (JSON) for uint16 range
        structOfPrimitives.setUInt16MinElement((short) 0);
        structOfPrimitives.setUInt16MaxElement(Short.MAX_VALUE);

        // negative values are not allowed on the wire (JSON) for uint32 range
        structOfPrimitives.setUInt32MinElement(0);
        structOfPrimitives.setUInt32MaxElement(Integer.MAX_VALUE);
        // structOfPrimitives.setUInt32MaxElement(new Long(4294967295L).intValue());

        // negative values are not allowed on the wire (JSON) for uint64 range
        structOfPrimitives.setUInt64MinElement(0L);
        if (useRestricted64BitRange) {
            // Javascript mastructOfPrimitives safe value
            structOfPrimitives.setUInt64MaxElement(9007199254740991L);
        } else {
            // Java and C++
            structOfPrimitives.setUInt64MaxElement(Long.MAX_VALUE);
        }

        Boolean[] bList = new Boolean[2];
        bList[0] = true;
        bList[1] = false;
        structOfPrimitives.setBooleanArray(bList);

        Double[] dList = new Double[2];
        dList[0] = 1.1d;
        dList[1] = 2.2d;
        structOfPrimitives.setDoubleArray(dList);

        Float[] fList = new Float[2];
        fList[0] = 1.1f;
        fList[1] = 2.2f;
        structOfPrimitives.setFloatArray(fList);

        Short[] i16List = new Short[2];
        i16List[0] = 1;
        i16List[1] = 2;
        structOfPrimitives.setInt16Array(i16List);

        Integer[] i32List = new Integer[2];
        i32List[0] = 1;
        i32List[1] = 2;
        structOfPrimitives.setInt32Array(i32List);

        Long[] i64List = new Long[2];
        i64List[0] = 1L;
        i64List[1] = 2L;
        structOfPrimitives.setInt64Array(i64List);

        Byte[] byteList = new Byte[2];
        byteList[0] = (byte) 1;
        byteList[1] = (byte) 2;
        structOfPrimitives.setInt8Array(byteList);

        String[] stringList = new String[2];
        stringList[0] = "Hello";
        stringList[1] = "World";
        structOfPrimitives.setStringArray(stringList);

        Short[] ui16List = new Short[2];
        ui16List[0] = 1;
        ui16List[1] = 2;
        structOfPrimitives.setUInt16Array(ui16List);

        Integer[] ui32List = new Integer[2];
        ui32List[0] = 1;
        ui32List[1] = 2;
        structOfPrimitives.setUInt32Array(ui32List);

        Long[] ui64List = new Long[2];
        ui64List[0] = 1L;
        ui64List[1] = 2L;
        structOfPrimitives.setUInt64Array(ui64List);

        Byte[] ui8List = new Byte[2];
        ui8List[0] = (byte) 1;
        ui8List[1] = (byte) 2;
        structOfPrimitives.setUInt8Array(ui8List);

        if (!checkStructOfPrimitives(structOfPrimitives)) {
            throw new RuntimeException("Internal error in checkStructOfPrimitives");
        }
        return structOfPrimitives;
    }

    public static StructOfPrimitives createStructOfPrimitives() {
        StructOfPrimitives structOfPrimitives = new StructOfPrimitives();
        fillStructOfPrimitives(structOfPrimitives);
        return structOfPrimitives;
    }

    /*
     * NOTE:
     * Currently the Java generator uses some bad datatypes.
     * Franca types "int16" and "uint16" are translated to Java "Integer"
     * while it should be "short"
     * Franca type "float" is translated to Java Double while it should
     * be "float".
     * All unsigned Franca types "uint8", "uint16", "uint32" and "uint64"
     * are translated to (signed) Java types which cannot hold the value
     * range. Instead those unsigned values are communicated over JSON
     * as if they were signed values of the same type.
     * I.e. the "uint8" value 255 is translated to "-1",
     * "uint16" value 65535 is translated to "-1" and so on.
     * It is subject of the user to translate these values back to
     * unsigned values hold by variable of type with larger range if
     * required.
     * The JSON file content is thus not identical to the really transferred
     * information for unsigned types.
     */
    @SuppressWarnings("checkstyle:methodlength")
    public static boolean checkStructOfPrimitives(StructOfPrimitives structOfPrimitives) {
        if (structOfPrimitives.getBooleanElement() != true) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter booleanElement "
                    + structOfPrimitives.getBooleanElement());
            return false;
        }

        if (!cmpDouble(structOfPrimitives.getDoubleElement(), 1.1d)) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter doubleElement "
                    + structOfPrimitives.getDoubleElement());
            return false;
        }

        if (!cmpFloat(structOfPrimitives.getFloatElement(), 1.1f)) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter floatElement "
                    + structOfPrimitives.getFloatElement());
            return false;
        }

        if (structOfPrimitives.getInt8MinElement() != Byte.MIN_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter int8MinElement "
                    + structOfPrimitives.getInt8MinElement());
            return false;
        }

        if (structOfPrimitives.getInt8MaxElement() != Byte.MAX_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter int8MaxElement "
                    + structOfPrimitives.getInt8MaxElement());
            return false;
        }

        if (structOfPrimitives.getInt16MinElement() != (int) Short.MIN_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter int16MinElement "
                    + structOfPrimitives.getInt16MinElement());
            return false;
        }

        if (structOfPrimitives.getInt32MinElement() != Integer.MIN_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter int32MinElement "
                    + structOfPrimitives.getInt32MinElement());
            return false;
        }

        if (structOfPrimitives.getInt32MaxElement() != Integer.MAX_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter int32MaxElement "
                    + structOfPrimitives.getInt32MaxElement());
            return false;
        }

        if (useRestricted64BitRange) {
            // Due to Javascript restrictions we cannot use the full 64-bit
            // value range here. The values have thus been reduced to the safe
            // integer number range of Javascript.
            if (structOfPrimitives.getInt64MinElement() != -9007199254740991L) {
                LOG.warn("methodWithMultipleStructParameters: invalid parameter int64MinElement "
                        + structOfPrimitives.getInt64MinElement());
                return false;
            }

            if (structOfPrimitives.getInt64MaxElement() != 9007199254740991L) {
                LOG.warn("methodWithMultipleStructParameters: invalid parameter int64MaxElement "
                        + structOfPrimitives.getInt64MaxElement());
                return false;
            }
        } else {
            // Java and C++
            if (structOfPrimitives.getInt64MinElement() != Long.MIN_VALUE) {
                LOG.warn("methodWithMultipleStructParameters: invalid parameter int64MinElement "
                        + structOfPrimitives.getInt64MinElement());
                return false;
            }

            if (structOfPrimitives.getInt64MaxElement() != Long.MAX_VALUE) {
                LOG.warn("methodWithMultipleStructParameters: invalid parameter int64MaxElement "
                        + structOfPrimitives.getInt64MaxElement());
                return false;
            }
        }

        if (!structOfPrimitives.getConstString().equals("Hiya")) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter constString "
                    + structOfPrimitives.getConstString());
            return false;
        }

        if (structOfPrimitives.getUInt8MinElement() != (byte) 0) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt8MinElement "
                    + structOfPrimitives.getUInt8MinElement());
            return false;
        }

        if (structOfPrimitives.getUInt8MaxElement() != Byte.MAX_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt8MaxElement "
                    + structOfPrimitives.getUInt8MaxElement());
            return false;
        }

        if (structOfPrimitives.getUInt16MinElement() != 0) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt16MinElement "
                    + structOfPrimitives.getUInt16MinElement());
            return false;
        }

        if (structOfPrimitives.getUInt16MaxElement() != Short.MAX_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt16MaxElement "
                    + structOfPrimitives.getUInt16MaxElement());
            return false;
        }

        if (structOfPrimitives.getUInt32MinElement() != 0) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt32MinElement "
                    + structOfPrimitives.getUInt32MinElement());
            return false;
        }

        if (structOfPrimitives.getUInt32MaxElement() != Integer.MAX_VALUE) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt32MaxElement "
                    + structOfPrimitives.getUInt32MaxElement());
            return false;
        }

        if (structOfPrimitives.getUInt64MinElement() != 0L) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt64MinElement "
                    + structOfPrimitives.getUInt64MinElement());
            return false;
        }

        //
        // TODO: The mastructOfPrimitivesimum safe integer value of Javascript is
        // 2^53-1 == 9007199254740991. Unfortunately this number
        // is way below 2^63-1 == 9223372036854775807 so that
        // it will still always reside in the positive range of both
        // signed and unsigned 64-bit values.
        // A real test for unsigned 64-bit values which works
        // for all 3 programming languages in parallel thus cannot
        // be implemented since it would require a positive number
        // greater than 2^63.
        //
        // Once the serialization format changes from 'always signed'
        // number format to 'signed or unsigned' large values
        // cannot be used anymore.
        //

        if (useRestricted64BitRange) {
            // Javascript
            if (structOfPrimitives.getUInt64MaxElement() != 9007199254740991L) {
                LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt64MaxElement "
                        + structOfPrimitives.getUInt64MaxElement());
                return false;
            }
        } else {
            // Java and C++
            if (structOfPrimitives.getUInt64MaxElement() != Long.MAX_VALUE) {
                LOG.warn("methodWithMultipleStructParameters: invalid parameter uInt64MaxElement "
                        + structOfPrimitives.getUInt64MaxElement());
                return false;
            }
        }

        if (structOfPrimitives.getBooleanArray().length != 2 || structOfPrimitives.getBooleanArray()[0] != true
                || structOfPrimitives.getBooleanArray()[1] != false) {
            LOG.warn("invalid parameter structOfPrimitives: invalid boolean array");
            return false;
        }

        if (structOfPrimitives.getDoubleArray().length != 2 || !cmpDouble(structOfPrimitives.getDoubleArray()[0], 1.1d)
                || !cmpDouble(structOfPrimitives.getDoubleArray()[1], 2.2d)) {
            LOG.warn("invalid parameter structOfPrimitives: invalid double array");
            return false;
        }

        if (structOfPrimitives.getFloatArray().length != 2 || !cmpFloat(structOfPrimitives.getFloatArray()[0], 1.1f)
                || !cmpFloat(structOfPrimitives.getFloatArray()[1], 2.2f)) {
            LOG.warn("invalid parameter structOfPrimitives: invalid float array");
            return false;
        }

        if (structOfPrimitives.getInt16Array().length != 2 || structOfPrimitives.getInt16Array()[0] != 1
                || structOfPrimitives.getInt16Array()[1] != 2) {
            LOG.warn("invalid parameter structOfPrimitives: invalid int16 array");
            return false;
        }

        if (structOfPrimitives.getInt32Array().length != 2 || structOfPrimitives.getInt32Array()[0] != 1
                || structOfPrimitives.getInt32Array()[1] != 2) {
            LOG.warn("invalid parameter structOfPrimitives: invalid int32 array");
            return false;
        }

        if (structOfPrimitives.getInt64Array().length != 2 || structOfPrimitives.getInt64Array()[0] != 1L
                || structOfPrimitives.getInt64Array()[1] != 2L) {
            LOG.warn("invalid parameter structOfPrimitives: invalid int64 array");
            return false;
        }

        if (structOfPrimitives.getInt8Array().length != 2 || structOfPrimitives.getInt8Array()[0] != (byte) 1
                || structOfPrimitives.getInt8Array()[1] != (byte) 2) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter structOfPrimitives: invalid int8 array");
            return false;
        }

        if (structOfPrimitives.getStringArray().length != 2 || !structOfPrimitives.getStringArray()[0].equals("Hello")
                || !structOfPrimitives.getStringArray()[1].equals("World")) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter structOfPrimitives: invalid string array");
            return false;
        }

        if (structOfPrimitives.getUInt16Array().length != 2 || structOfPrimitives.getUInt16Array()[0] != 1
                || structOfPrimitives.getUInt16Array()[1] != 2) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter structOfPrimitives: invalid uint16 array");
            return false;
        }

        if (structOfPrimitives.getUInt32Array().length != 2 || structOfPrimitives.getUInt32Array()[0] != 1
                || structOfPrimitives.getUInt32Array()[1] != 2) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter structOfPrimitives: invalid uint32 array");
            return false;
        }

        if (structOfPrimitives.getUInt64Array().length != 2 || structOfPrimitives.getUInt64Array()[0] != 1L
                || structOfPrimitives.getUInt64Array()[1] != 2L) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter structOfPrimitives: invalid uint64 array");
            return false;
        }

        if (structOfPrimitives.getUInt8Array().length != 2 || structOfPrimitives.getUInt8Array()[0] != (byte) 1
                || structOfPrimitives.getUInt8Array()[1] != (byte) 2) {
            LOG.warn("methodWithMultipleStructParameters: invalid parameter structOfPrimitives: invalid uint8 array");
            return false;
        }
        return true;
    }

    // TYPE ExtendedStructOfPrimitives extends StructOfPrimitives
    public static ExtendedStructOfPrimitives fillExtendedStructOfPrimitives(ExtendedStructOfPrimitives extendedStructOfPrimitives) {
        // ExtendedTypeCollectionEnumerationInTypeCollection
        extendedStructOfPrimitives.setExtendedEnumElement(ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
        extendedStructOfPrimitives.setExtendedStructElement(createExtendedBaseStruct());
        fillStructOfPrimitives(extendedStructOfPrimitives);
        if (!checkExtendedStructOfPrimitives(extendedStructOfPrimitives)) {
            throw new RuntimeException("Internal error in checkExtendedStructOfPrimitives");
        }
        return extendedStructOfPrimitives;
    }

    public static ExtendedStructOfPrimitives createExtendedStructOfPrimitives() {
        ExtendedStructOfPrimitives extendedStructOfPrimitives = new ExtendedStructOfPrimitives();
        fillExtendedStructOfPrimitives(extendedStructOfPrimitives);
        return extendedStructOfPrimitives;
    }

    public static boolean checkExtendedStructOfPrimitives(ExtendedStructOfPrimitives extendedStructOfPrimitives) {
        if (extendedStructOfPrimitives.getExtendedEnumElement() != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            return false;
        }

        // check inherited parts
        if (extendedStructOfPrimitives.getExtendedStructElement().getEnumElement() != Enumeration.ENUM_0_VALUE_3
                || (!extendedStructOfPrimitives.getExtendedStructElement().getBaseStructString().equals("Hiya"))) {
            return false;
        }

        if (!checkStructOfPrimitives(extendedStructOfPrimitives)) {
            return false;
        }
        return true;
    }

    public static boolean cmpFloat(float a, float b) {
        return Math.abs(a - b) < 0.001;
    }

    public static boolean cmpDouble(double a, double b) {
        return Math.abs(a - b) < 0.001;
    }

    public static Byte[] concatenateByteArrays(Byte[] array1, Byte[] array2) {
        int array1length = array1.length;
        int array2length = array2.length;

        Byte[] concatenatedArray = new Byte[array1length + array2length];
        System.arraycopy(array1, 0, concatenatedArray, 0, array1length);
        System.arraycopy(array2, 0, concatenatedArray, array1length, array2length);
        return concatenatedArray;
    }

}
