module.exports = {
    transform: {
        "\\.ts$": "ts-jest"
    },
    testEnvironment: "node",
    testRegex: "src/main/js/.*\\.spec\\.ts$",
    testPathIgnorePatterns: ["/node_modules/"],
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
    modulePathIgnorePatterns: ["target"],
    testRunner: "jest-circus/runner"
};
