# joynr Franca C++ Generator
## Core Purpose
The Franca Model C++ Code Generator transforms Franca Interface Definition Language (IDL) files into C++ source code. It automates boilerplate generation, ensures consistency, and supports rapid development by creating stubs and bindings for communication protocols.

## Project Structure

See Diagrams below for visualisation.

```bash
/src/tools/generator/framework
    /src/main/xtend/io/joynr/generator/templates/util
                                       - Utilities common to all code generators
                       
/src/tools/generator/cpp-generator         – C++ source code for the generator
    /communicationmodel                    - see .puml for visualization
        /CommunicationModelGenerator.xtend - generate the boilerplate data types
        /TypeCppTemplate.xtend             - generate the types using injected
                                             classes for enum and map
        /TypeHTemplate.xtend               - header generation
        /TypeDefHTemplate.xtend            - generate the C++ typedef alias
        /EnumCppTemplate.xtend             - generate Enum data types
        /EnumHTemplate.xtend               - header generation
        /MapCppTemplate.xtend              - generate Map data types
        /MapHTemplate.xtend                - header generation
    /proxy
        /ProxyGenerator.xtend             - generate proxy in different
                                             variations, listed below
        /IInterfaceConnectorHTemplate.xtend
        /InterfaceAsyncProxyCppTemplate.xtend
        /InterfaceAsyncProxyHTemplate.xtend
        /InterfaceFireAndForgetProxyCppTemplate.xtend
        /InterfaceFireAndForgetProxyHTemplate.xtend
        /InterfaceProxyBaseCppTemplate.xtend
        /InterfaceProxyBaseHTemplate.xtend
        /InterfaceProxyCppTemplate.xtend
        /InterfaceProxyHTemplate.xtend
        /InterfaceSyncProxyCppTemplate.xtend
        /InterfaceSyncProxyHTemplate.xtend
    /defaultProvider                       - generates default provider
        /DefaultInterfaceProviderGenerator.xtend
        /DefaultInterfaceProviderCppTemplate.xtend
        /DefaultInterfaceProviderHTemplate.xtend
    /provider
        /ProviderGenerator.xtend          - generates the provider logic
        /InterfaceAbstractProviderCppTemplate.xtend
        /InterfaceAbstractProviderHTemplate.xtend
        /InterfaceProviderCppTemplate.xtend
        /InterfaceProviderHTemplate.xtend
        /InterfaceRequestCallerCppTemplate.xtend
        /InterfaceRequestCallerHTemplate.xtend
        /InterfaceRequestInterpreterCppTemplate.xtend
        /InterfaceRequestInterpreterHTemplate.xtend
    /filter                               - the filter or
                                            the broadcast filter parameters
        /FilterGenerator.xtend
        /FilterParameterTemplate.xtend
        /FilterTemplate.xtend
    /interfaces                           - interfaces (broadcast, reply)
        /InterfaceCppTemplate.xtend
        /InterfaceGenerator.xtend
        /InterfaceHTemplate.xtend
    /joynrmessaging                       - messaging (messaging boiler plate code)
        /InterfaceJoynrMessagingConnectorCppTemplate.xtend
        /InterfaceJoynrMessagingConnectorHTemplate.xtend
        /JoynrMessagingGenerator.xtend

        
    
    /src/test/java          - Unit and integration tests
    /src/test/resources     - Templates for test
    /pom.xml                - Maven build configuration
```

## Prerequisites
Build System: Maven or Gradle

Languages: C++, Xtend

Frameworks: Guice (dependency injection), Franca (IDL parsing)

IDE: IntelliJ IDEA or VSC

## How It Works
1. Parse .fidl files using the Franca parser.
2. Process the AST (Abstract Syntax Tree).
3. Apply templates (e.g., Xtend, Freemarker) to generate code.
4. Write generated files to the target directory.

## Usage Instructions
Command Line:

in cpp generator root run:

```bash
mvn clean install
```

build the cpp app using joynr dependencies
example for radio app included in joynr project

```bash
cd examples/radio-app
mvn clean install
```

## Template Design
Example:
Xtend Template Example:

Copy code
```xtend
«FOR iface : model.interfaces»
public interface «iface.name» {
    «FOR op : iface.operations»
    «op.returnType» «op.name»(«op.params.join(", ")»);
    «ENDFOR»
}
«ENDFOR»
```

Template Guidelines:

Use meaningful variable names (e.g., iface, op).

Document complex expressions or loops with comments.

Follow consistent formatting for generated code.
