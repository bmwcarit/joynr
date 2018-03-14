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
                <!-- choose the generation language -->
                <generationLanguage><GENERATION_LANGUAGE></generationLanguage>
                <!-- specify the output directory -->
                <outputPath><PATH_TO_OUTPUT_DIRECTORY></outputPath>
                <!-- optional parameters -->
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
        <!-- For Javai/Jee code generation:
             add the Java/Jee generator dependency -->
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

        <!-- For JEE code generation:
             add the JEE generator dependency -->
        <dependency>
            <groupId>io.joynr.tools.generator</groupId>
            <artifactId>java-generator</artifactId>
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


## Choosing the generation language
The **&lt;GENERATION_LANGUAGE&gt;** can be either ```java```, ```cpp```, or ```javascript```.
In each case, the corresponding dependency has to be added to the plugin's dependencies
section (see above):
* for ```java```: the artifact **io.joynr.tools.generator: java-generator**
* for ```cpp```: the artifact **io.joynr.tools.generator: cpp-generator**
* for ```javascript```: the artifact **io.joynr.tools.generator: js-generator**


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


## Joynr Generator Standalone
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
    Optional, C++ only:
      -outputHeaderPath <path to directory containing header files>
      -includePrefix <prefix to use in include statements>
    Optional, JS only:
      -requireJSSupport <true, false>
        true: generate exports for all require mechanisms such as requirejs, browser and node
        false: generate only module.exports for node
        default: false
```

