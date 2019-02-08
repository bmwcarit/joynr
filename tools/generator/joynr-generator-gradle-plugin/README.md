# Joynr Generator Gradle Plugin

This plugin can be used to generate source files from fidl files,
similar to the corresponding maven plugin(joynr-generator-maven-plugin).

For documentation on how to use this plugin, see the [generator documentation](../../../wiki/generator.md).

## Build

The plugin itself is built using gradle.
The gradle wrapper is already added for convenience.

```bash
gradle build
# or
./gradlew build  # alternative using the gradle wrapper
```

To publish to your local maven repository use:

```bash
gradle publishToMavenLocal
# or
./gradlew publishToMavenLocal  # alternative using the gradle wrapper
```