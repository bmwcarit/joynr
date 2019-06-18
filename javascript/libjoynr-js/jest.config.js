module.exports = {
    transform: {
        "\\.ts$": "ts-jest"
    },
    testEnvironment: "node",
    testRegex: "src/test/.*Test\\.ts$",
    moduleFileExtensions: ["ts", "js", "json", "node"],
    reporters: [
        "default",
        [
            "jest-junit",
            {
                output: "target/test-results/ts-junit.xml"
            }
        ]
    ],
    coverageThreshold: {
        "src/**/*.ts": {
            branches: 80,
            functions: 80,
            lines: 80,
            statements: 80
        }
    },
    coverageDirectory: ".build/coverage/ts",
    collectCoverageFrom: ["src/main/js/**/*.ts", "!src/**/*.d.ts"],
    coveragePathIgnorePatterns: ["/src/main/js/generated", "/node_modules"],
    coverageReporters: ["cobertura", "lcov"],
    modulePathIgnorePatterns: ["target"]
};
