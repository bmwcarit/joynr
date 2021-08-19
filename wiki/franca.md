# Franca IDL overview

Franca IDL files can be written in any text editor.

We recommend using the Franca Eclipse tooling to simplify the process (syntax highlighting, code
completion, model validation etc.): Franca IDL (https://github.com/franca/franca)

The following sections provide a brief overview of Franca.

## Franca Data types
### Basic data types
The following Franca basic data types are supported by joynr:

Name | Meaning
-----|--------
*Int8*, *Int16*, *Int32*, *Int64* | signed 8, 16, 32, 64-bit integer value
*UInt8*, *UInt16*, *UInt32*, *UInt64* | unsigned 8, 16, 32, 64-bit integer value
*Boolean* | boolean value
*Float*, *Double* | single / double precision floating point number
*String* | String
*ByteBuffer* | buffer of bytes

### Complex data types

The following complex data types are supported by joynr:

Name | Meaning
-----|--------
*struct* *<Type>* { *elements* } | Data record
*enumeration* *<Type>* { *values* } | Enumeration
*map* *<Type>* { *KeyType* to *ValueType* } | associative array, hash
*type[]* | inline array

Each *element* is a ```<Type> variableName``` pair.

### Restrictions

Please be aware of several restrictions:

* Java does not support any unsigned types at all, always use signed instead if a Java application
  is part of the communication.
* JavaScript does not support `int64` or `uint64`. It has a [limited maximum integer that is safe
  to use](https://developer.mozilla.org/de/docs/Web/JavaScript/Reference/Global_Objects/Number/MAX_SAFE_INTEGER)
* C++ does not support `NULL` values or undefined values. We strongly discourage the use of `NULL`
  for all languages. If you want to emulate optional elements, you should instead use an array. An
  empty array would then signal the absence of a value, whereas the presence of a value would be
  signalled by the array having one element.
* The serialization of a `ByteBuffer` is extremely inefficient. We recommend instead encoding the
  data with Base64 and putting it in a `String` value.

## Franca Packages

A Franca package definition creates a unique namespace.

```
package <Package>
```

## Franca Type Collections

Type collections serve as **namespace** for one or more named custom complex types.

```
typeCollection <TypeCollection> {
    ... complex type declarations ...
}
```

**It is recommended to use only one type collection in joynr**, since at the moment the names of the types in all type collections must be unique.

Outside of a type collection, data types are referenced using hierachical notation like ```<TypeCollection>.<complexType>```.

## Franca Interfaces

A Franca interface defines a namespace for **attributes**, **methods** and **broadcasts**, which must be **defined in exactly that order**.

```
interface <Interface> {
    ... attributes (optional) ...
    ... methods (optional)  ...
    ... broadcasts (optional) ...
}
```

The following restrictions apply for joynr:

* Franca **Contracts** are not supported.
* For a given interface, all attribute, method and broadcast **names must be unique**, e.g. a name for an attribute may not be reused as name for a method or broadcast etc.
* While technically data types can be defined inside an interface as well, it is strongly recommended to place them inside a type collection.

### Attributes

Attributes are values to which a consumer may subscribe, in order to get informed upon changes and/or periodically via a publication.

```
attribute <Type> <Attribute>
```

### Methods

A Franca method defines an RPC call that can be invoked by a Consumer application. The call is then sent to and handled by a Provider application.

RPC supports optional input- and output parameters as well as an optional enumeration for error codes.

For Java and C++, the consumer may make synchronous or asynchronous calls to a proxy.

```
method <Method> {
    in {
        ... collection of "<Type> <inputVariable>" pairs ...
    }
    out {
        ... collection of "<returnType> <outputVariable>" pairs ...
    }
    error <enumerationType>
}
```

### Broadcasts

A Franca broadcast defines an asynchronous event that can be fired by the Provider and be received by the Consumer.
A consumer can selectively subscribe to desired events, optionally with filter parameters to limit the number of broadcasts it gets.
Typically broadcasts have optional output parameter(s) to provide details about the event.

```
broadcast <Broadcast> {
    out {
        <Type> <outputVariable>
    }
}
```

### Selective (filtered) broadcasts

If a broadcast supports filtering, the filter parameter must be defined using Franca comments as follows:

```
<**
    @description: <description text>
    @param: <filterParam1> (<filterParamType1>) descriptionParam1
    ...
    @param: <filterParamN> (<filterParamTypeN>) descriptionParamN
**>
broadcast <Broadcast> selective {
    ...
    out {
        ...
    }
}
```
Important things to notice for the declaration of selective broadcasts:
* Each description text can hold multiple lines.
* The parenthesis in the parameters definition do not mean that the type information is optional, but rather mean
that it has indeed to be provided inbetween parenthesis, e.g. (Boolean) or (String) etc.
* The keyword `selective`

# Example

```
package myPackage
typeCollection myTypeCollection {
    struct myStruct {
        String myElement1
        String myElement2
    }
}

interface myInterface {
    method myMethod {
        in {
            myTypeCollection.myStruct myInput
        }
        out {
            String myOutput
        }
        error {
            ERROR_VALUE_1
            ...
        }
    }

    broadcast myBroadcast {
        out {
            myTypeCollection.myStruct myOutput
        }
    }

    <**
        @param: filterMinValue (Integer) filter parameter
                that defines the minimal value of interest
                for the subscriber
    **>
    broadcast mySelectiveBroadcast selective {
        out {
            Integer myIntValue
        }
    }
}
```

## Further Reading
[Franca IDL documentation on GitHub (external link: https://github.com/franca/franca)](https://github.com/franca/franca)

For details about using joynr with Java see [joynr Java Developer Guide](java.md).
For details about using joynr with C++ see [joynr C\+\+ Developer Guide](cplusplus.md).
For details about using joynr with JavaScript see [joynr JavaScript Developer Guide](javascript.md).
