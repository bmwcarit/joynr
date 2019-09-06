/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

// eslint-disable-next-line @typescript-eslint/no-var-requires
const path = require("path");

export default function transformer(file, api) {
    const j = api.jscodeshift;
    const ast = j(file.source);
    const libjoynrRoot = path.join(__dirname, "../src/main/js");
    const joynrLength = "joynr/".length;

    const fileDir = path.dirname(file.path);

    ast.find(j.TSExternalModuleReference).forEach(nodePath => {
        const literal = nodePath.value.expression.value;
        if (literal.startsWith("joynr/joynr/")) {
            const absoluteFilePath = path.join(libjoynrRoot, literal.slice(joynrLength));
            const changedPath = path.relative(fileDir, absoluteFilePath);
            nodePath.replace(j.tsExternalModuleReference(j.stringLiteral(changedPath)));
        }
    });

    ast.find(j.ImportDeclaration).forEach(nodePath => {
        const literal = nodePath.value.source.value;
        if (literal.startsWith("joynr/joynr/")) {
            const absoluteFilePath = path.join(libjoynrRoot, literal.slice(joynrLength));
            const changedPath = path.relative(fileDir, absoluteFilePath);
            nodePath.replace(
                j.importDeclaration(nodePath.value.specifiers, j.stringLiteral(changedPath), nodePath.value.importKind)
            );
        }
    });

    return ast.toSource();
}
