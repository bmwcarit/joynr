#The joynr Code Generator
The joynr Code Generator can be used to create **Java** and **C++** Code from Franca model files (*.fidl). Code generation itself is integrated into the Maven build process. See also the Demo App tutorial and sample code.


## Maven configuration
The **output path** for the generated code, the **model files**, and the **target language** have to be provided in the Maven configuration:
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
            </configuration>
        </execution>
    </executions>
    <dependencies>
        <!-- For Java code generation:
             add the Java generator dependency -->
        <dependency>
            <groupId>io.joynr.java</groupId>
            <artifactId>java-generator</artifactId>
            <version><JOYNR_VERSION></version>
        </dependency>

        <!-- For C++ code generation:
             add the C++ generator dependency -->
        <dependency>
            <groupId>io.joynr.cpp</groupId>
            <artifactId>cpp-generator</artifactId>
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
The **&lt;GENERATION_LANGUAGE&gt;** can be either ```java``` or ```cpp```. In each case, the corresponding dependency has to be added to the plugin's dependencies section (see above):
* for ```java```: the artifact **io.joynr.cpp: cpp-generator**
* for ```cpp```: the artifact **io.joynr.java: java-generator**


## Providing the Franca model files
The model (Franca files) can either be provided by a **relative path on the file system** or **file on the class path**. The relative path can directly **name a Franca file or point to a folder**. In the latter case, all Franca files in the folder are used for generation. If a file on the classpath is referenced, the model must be in the plugin's dependencies section (see above).

The dependencies section can also be used to provide and reuse non app specific Franca files, e.g. simple data types, that are then imported in the local Franca files.

It is recommended to place any local model files into the project's subdirectory ```src/main/model``` like in the Demo application.


## Joynr Generator Standalone
The joynr Code Generator also exists as standalone version which can be used to manually generate the code from the Franca model.

The standalone version as well as the Maven plugin are produced during the joynr Java build (see [Building joynr Java and common components](building_joynr_java.md)) and installed into the local Maven repository. The standalone jar resides in the target directory in *&lt;JOYNR_REPOSITORY&gt;/tools/generator/joynr-generator-standalone*. It has to be called with the following command line arguments:
* ```-outputPath```: output directory for the generated code
* ```-modelpath```: path to the Franca model files
* ```-generationLanguage```: either ```java``` or ```cpp```
