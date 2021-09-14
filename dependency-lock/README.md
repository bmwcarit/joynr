# Maven dependency management

The joynr Maven direct dependency versions are managed in the `dependencyManagement` section of
the [root POM](../pom.xml).
The direct dependencies shall use fixed versions and not version ranges.

The direct and transitive dependency versions are locked in [lock POM](./pom.xml).

In case automatic Maven dependency mediation results in undesired versions,
these transitive dependencies shall also be managed in the [root POM](../pom.xml),
at the end of the `dependencyManagement` section. Comment these manually managed transitive
dependencies, explaining:
* why the automatic Maven dependency mediation result cannot be used
* which direct dependencies causing the problem (if any)

## Changes
In case direct dependencies are added, removed or require a version change, follow the following
steps:
1. Add/Remove direct dependencies to/from the corresponding projects. Assure that the scope is
restrictive (use `runtime` or `test` if appropriate). Do not specify a version.
2. Add/Remove/Change the direct dependency version values (not range) to the `dependencyManagement`
section in the [root POM](../pom.xml). E.g. the latest available versions for JVM 11+ are used.
3. Use the tools described in the following section to update the [lock POM](./pom.xml).

### Radio App
The [../examples/radio-app/pom.xml](Radio App POM) is used as an example POM file for user
applications. Hence it should be independent from the [root POM](../pom.xml). All
direct dependencies within that POM must have a version. To avoid implicit dependency locking,
do not specify a fixed version, but a version range as required by the APIs of the dependencies.

## Tools
The following tools are provided within this directory to support the dependency maintenance.
All tools require Python 3 and can be executed directly in Linux shells, without any arguments.

### write.py
Rewrites [lock POM](./pom.xml) using automatic Maven dependency mediation.
Rewriting a dependency lock can always lead to different results in case new versions are available
for dependencies not explicitly locked in the [root POM](../pom.xml). If a general transitive
dependency upgrade is not desired, try the `update.py` instead.

### update.py
Instead of a rewrite, the update uses the current version of [lock POM] before applying automatic
Maven dependency mediation. Hence only unused dependencies are removed and new dependencies are
added. Be aware that the tool is running on the [lock POM](./pom.xml) of your current workspace.
If you have modified it before by running `write.py`, revert the changes before running `update.py`.


### check.py
The script basically runs `update.py` and returns non-zero code if the new [lock POM](./pom.xml)
differs from the previous one. Note that the script does not alter your workspace content.
It can be safely used by CI/CD pipelines.
