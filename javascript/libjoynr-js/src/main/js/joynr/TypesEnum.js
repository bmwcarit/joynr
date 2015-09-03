/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define("joynr/TypesEnum", [], function() {

    /**
     * Is used in generated Proxies, Providers for Typing
     * @exports TypesEnum
     */
    var TypesEnum = {};

    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.BOOL = "Boolean";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.BYTE = "Byte";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.SHORT = "Short";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.INT = "Integer";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.LONG = "Long";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.FLOAT = "Float";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.DOUBLE = "Double";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.STRING = "String";
    /**
     * @type {String}
     * @member
     * @readonly
     */
    TypesEnum.LIST = "List";

    /**
     * Map of the primitive types
     * @type Object
     * @member
     * @readonly
     */
    TypesEnum.primitiveTypes = {};
    TypesEnum.primitiveTypes[TypesEnum.BOOL] = true;
    TypesEnum.primitiveTypes[TypesEnum.BYTE] = true;
    TypesEnum.primitiveTypes[TypesEnum.SHORT] = true;
    TypesEnum.primitiveTypes[TypesEnum.INT] = true;
    TypesEnum.primitiveTypes[TypesEnum.LONG] = true;
    TypesEnum.primitiveTypes[TypesEnum.FLOAT] = true;
    TypesEnum.primitiveTypes[TypesEnum.DOUBLE] = true;
    TypesEnum.primitiveTypes[TypesEnum.STRING] = true;

    return Object.freeze(TypesEnum);

});