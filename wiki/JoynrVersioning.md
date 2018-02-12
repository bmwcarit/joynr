# Joynr Versioning

## Semantic versioning of joynr
Starting with [joynr 1.0.0](ReleaseNotes.md) joynr follows the semantic
versioning scheme as defined at https://semver.org. Considering **x.y.z** as a
generic version with **x = MAJOR**, **y = MINOR** and **z = PATCH**, the
following will hold true:
* **(x+1)**.0.0 breaks API and/or the messaging layer.
* x.**(y+1)**.0 brings new features (also new APIs) but current API stays
stable. No change to the messaging layer.
* x.y.**(z+1)** this new version brings only bug fixes.

## joynr version on develop branch
As we do not expect nor want to break current API or the current messaging
layer, after any release, develop is automatically updated to the subsequent
**MINOR** release with the appending word **SNAPSHOT** attached to it. So for
example, after releasing joynr **1.0.0** the develop branch is updated to
**1.1.0-SNAPSHOT**. For possible **1.0.z** releases that are merged into
develop the versioning on the branch is not updated.

If API breaks or messaging layer breaks are merged into develop then develop
is updated to the subsequent **MAJOR** version. So for example, if develop points
currently at **1.1.0-SNAPSHOT** after merging a messaging layer change, develop
is updated to **2.0.0-SNAPSHOT**. The version of develop will then stay the same
until the next major release branch.

Every non **PATCH** release will have a version which is equivalent to the one
on develop without the word **SNAPSHOT**. So for example, if develop points at
**2.0.0-SNAPSHOT** the next major release will be **2.0.0**.

## A note on ABI compatibility for C++ applications
C++ ABI compatibility is not explicitly part of this versioning scheme and is
generally not guaranteed across releases. A change in the **MINOR** or **MAJOR**
version should always be considered ABI incompatible. **PATCH** versions are
usually ABI compatible however, if we suspect that the next **PATCH** version is not
ABI compatible, the release notes will mention it.


# License
These specifications are derived from https://semver.org/spec/v2.0.0.html (version 2.0). This file
is hence released under the terms of the
[Creative Commons - CC BY 3.0](https://creativecommons.org/licenses/by/3.0).
