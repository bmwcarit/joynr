# The joynr Code Generator
The joynr Code Generator can be used to create **Java**, **C++** and **Javascript** code
from Franca model files (*.fidl). Code generation itself is integrated into the Maven
build process. See also the Demo App tutorial and sample code.


## Maven configuration
The **output path** for the generated code, the **model files**, and the **target language**
have to be provided in the Maven configuration:

```xml
<plugin>
    <groupId>io.joynr.tools.generator</groupId>
    <artifactId>joynr-generator-maven-plugin</artifactId>
    <executions>
        <execution>
            <id>generate-code</id>
            <phase>generate-sources</phase>
            <goals>
                <goal>generate</goal>
            </goals>
            <configuration>
                <!-- provide the Franca model file(s) -->
                <model><PATH_TO_MODEL_FILE_OR_DIRECTORY></model>
                <!-- choose the generation language
                        (see section "Choosing the generation language"):
                    either by specifying <generationLanguage>
                    or <rootGenerator> -->
                <generationLanguage><GENERATION_LANGUAGE></generationLanguage>
                <rootGenerator><FULL_NAME_OF_ROOT_GENERATOR></rootGenerator>
                <!-- specify the output directory -->
                <outputPath><PATH_TO_OUTPUT_DIRECTORY></outputPath>
                <!-- optional parameters -->
                <!-- specify how the major version of Franca interfaces and typecollections shall
                        affect the generated name and package of interfaces and types:
                    DEPRECATED! Set the #noVersionGeneration comment in the .fidl file instead,
                    see section "Disable versioning of generated files" below.
                    package: interface/typecollection major versions (if existing) are added as an
                        additional package segment
                    name: interface/typecollection major versions (if existing) are appended to the
                        interface/type name
                    none (default): interface/typecollection versions do not affect the generated
                        name and package of interfaces and types
                    NOTE:
                        - Consumer and provider applications of one interface have to use the same
                          versioning scheme to be able to communicate with each other!
                        - The feature has been fully tested to work in Java, in C++ and JS only the
                          versioning of interfaces has been tested so far but the versioning of types
                          is expected to work as well. -->
                <addVersionTo>name|package|none</addVersionTo>
                <parameter>
                    <!-- for Jee code generation use generation language "java"
                         and set the following parameter
                         (see also documentation of joynr JEE Integration -->
                    <jee>true</jee>
                    <!-- for Java/Jee code generation with null values in
                         complex types use generation language "java"
                         and set the following parameter -->
                    <ignoreInvalidNullClassMembers>true</ignoreInvalidNullClassMembers>
                    <!-- for JavaScript only:
                         requireJSSupport=true: generate exports for all require mechanisms such
                             as requirejs, browser and node
                         requireJSSupport = false: generate only module.exports for node
                         default: false -->
                    <requireJSSupport>true</requireJSSupport>
                </parameter>
            </configuration>
        </execution>
    </executions>
    <dependencies>
        <!-- For Java/JEE code generation:
             add the Java/JEE generator dependency -->
        <dependency>
            <groupId>io.joynr.tools.generator</groupId>
            <artifactId>java-generator</artifactId>
            <version><JOYNR_VERSION></version>
        </dependency>

        <!-- For C++ code generation:
             add the C++ generator dependency -->
        <dependency>
            <groupId>io.joynr.tools.generator</groupId>
            <artifactId>cpp-generator</artifactId>
            <version><JOYNR_VERSION></version>
        </dependency>

        <!-- For JavaScript code generation:
             add the JavaScript generator dependency -->
        <dependency>
            <groupId>io.joynr.tools.generator</groupId>
            <artifactId>js-generator</artifactId>
            <version><JOYNR_VERSION></version>
        </dependency>

        <!-- optionally, use model files from a dependency artifact -->
        <dependency>
            <groupId><MODEL_GROUP_ID></groupId>
            <artifactId><MODEL_ARTIFACT_ID></artifactId>
            <version><MODEL_VERSION></version>
        </dependency>
    </dependencies>
</plugin>
```

## Gradle configuration
To include the plugin in your `build.gradle` use

```java
buildscript {
    ...
    dependencies {
        classpath 'io.joynr.tools.generator:joynr-generator-gradle-plugin:<JOYNR_VERSION>'
        // Choose from the following depending on your target language
        classpath 'io.joynr.tools.generator:java-generator:<JOYNR_VERSION>'
        classpath 'io.joynr.tools.generator:cpp-generator:<JOYNR_VERSION>'
        classpath 'io.joynr.tools.generator:js-generator:<JOYNR_VERSION>'
    }
}

apply plugin: 'java'
apply plugin: 'io.joynr.tools.generator.joynr-generator-gradle-plugin'
```

Note: The joynr Generator Plugin has to be applied after the *Java* or *Kotlin*
plugin, as it attaches itself to the corresponding *clean* task.

The following parameters can be configured (see section [Maven
configuration](#maven-configuration) for details):

* `modelPath`: Defines the path where the `.fidl` files are created and located.
  You must place the models in this location. The path must always be specified
  from the project's root onwards, typically `app/src/...`. If not supplied in
  the script, default value is `src/main/fidl/` or `app/src/main/fidl/` depending on where this 
  is found first (if nowhere, default is `app/src/main/fidl/`). Note that in a multi-project 
  setup, paths must always be given from the root project onwards, i.e. 
  root-project/{sub-project1/...}; the path enclosed in {} is the path to specify.
* `outputPath`: The output path where the generated files are created. The
  default value is `build/generated/source/fidl/` or `app/build/generated/source/fidl/` depending
   on whether you have a multi-project setup (if the script can't find a suitable place, it will 
   fall back to `app/build/generated/source/fidl/`).
* `generationLanguage`: The language to be used for generator tool selection.
  The default value is `java`.
* `rootGenerator`
* `generationId`
* `skip`
* `addVersionTo` / DEPRECATED. The addVersionTo option will be removed
  in a future version of the generator. Set the #noVersionGeneration comment
  in the .fidl file instead, see [Disable versioning of generated files](#disable-versioning-of-generated-files).
* `extraParameters`

Example configuration:

```java
joynrGenerator {
    modelPath = "src/main/model/"
    outputPath = "src/main/generated-java/"
    generationLanguage = "java"
}
```

The plugin can now be invoked with

```bash
gradle joynr-generate
```

To remove the generated sources again, invoke

```bash
gradle clean
```

## Choosing the generation language
The value **&lt;GENERATION_LANGUAGE&gt;** of the setting `generationLanguage` can be either
`java`, `cpp`, or `javascript`.

In each case, the corresponding dependency has to be added to the plugin's dependencies
section (see above):
* for **java**: the artifact `io.joynr.tools.generator: java-generator`
* for **cpp**: the artifact `io.joynr.tools.generator: cpp-generator`
* for **javascript**: the artifact `io.joynr.tools.generator: js-generator`

Alternatively, the generation language can also be chosen by the setting `rootGenerator`.
Possible values of **&lt;FULL_NAME_OF_ROOT_GENERATOR&gt;** are
* for **java**: `io.joynr.generator.JoynrJavaGenerator`,
* for **cpp**: `io.joynr.generator.cpp.JoynrCppGenerator`,
* for **javascript**: `io.joynr.generator.js.JoynrJSGenerator`


## Providing the Franca model files
The model (Franca files) can either be provided by a **relative path to a file on the
filesystem** or a **relative path to a directory on the filesystem** or a **relative path
to a single file inside a JAR on the class path**. In case a directory on the filesystem
is specified, all Franca files in the directory are used for generation. If a file inside
a JAR on the classpath is referenced, the model artifact containing the referenced file must
be in the plugin's dependencies section (see above). To generate code for multiple files in
JAR files on the class path, each file has to be configured in its own execution section.

The dependencies section can also be used to provide and reuse non app specific Franca files,
e.g. simple data types, that are then imported in the local Franca files.

It is recommended to place any local model files into the project's subdirectory
```src/main/model``` like in the Demo application.


## Disable versioning of generated files
In the future, the generator generates package version information for interfaces and the
types used by an interface by default. Interface/typecollection major versions (defaults to
0 if not defined in the Franca file) are added as an additional package/namespace segment. 
To disable version generation, add a line containing #noVersionGeneration to the description
(Franca @description comment) of the interface:

```
<**
    @description: This is my Interface.
        #noVersionGeneration
**>
```

NOTE:
> The evaluation of #noVersionGeneration comment is not yet activated. A warning is printed if
> the comment and the addVersionTo setting do not match. The interface comment should be
> updated in this case to get the same generator output with future versions of the generator.

## joynr Generator Standalone
The joynr Code Generator also exists as standalone version which can be used to manually
generate the code from Franca models.

The standalone version as well as the Maven plugin are produced during the joynr Java build
(see [Building joynr Java and common components](java_building_joynr.md)) and installed into
the local Maven repository. The standalone jar resides in the target directory in
*&lt;JOYNR_REPOSITORY&gt;/tools/generator/joynr-generator-standalone*. It has to be called
with the following command line arguments:
* ```-outputPath```: output directory for the generated code
* ```-modelpath```: path to the Franca model files
* ```-generationLanguage```: either ```java```, ```cpp``` or ```javascript```

**Example:**
```bash
java -jar target/joynr-generator-standalone-0.9.0.jar
-outputPath gen -modelpath model.fidl -generationLanguage java
```
This will generate *Java* code for the model file *model.fidl* and output the generated code
to *gen*.

**Command line arguments:**
```
    Required:
      -modelpath <path to model>
      -outputPath <path to output directory>
    Also one of:
      -rootGenerator <full name of template root> OR
      -generationLanguage <cpp|java|javascript>
    Optional:
      -templatesDir <folder name of templates directory>
      -templatesEncoding <encoding of templates>
      -generationId <name of what is being generated>
      -addVersionTo <name, package, none>
        DEPRECATED! Set the #noVersionGeneration comment in the .fidl file instead,
        see section "Disable versioning of generated files" above.
        package: interface/typecollection major versions (if existing) are added as an additional
            package segment
        name: interface/typecollection major versions (if existing) are appended to the
            interface/type name
        none: interface/typecollection versions do not affect the generated name and package of
            interfaces and types
        default: none
        NOTE:
            - Consumer and provider applications of one interface have to use the same versioning
              scheme to be able to communicate with each other!
            - The feature has been fully tested to work in Java, in C++ and JS only the versioning
              of interfaces has been tested so far but the versioning of types is expected to work
              as well.
    Optional, C++ only:
      -outputHeaderPath <path to directory containing header files>
      -includePrefix <prefix to use in include statements>
    Optional, JS only:
      -requireJSSupport <true, false>
        true: generate exports for all require mechanisms such as requirejs, browser and node
        false: generate only module.exports for node
        default: false
```

