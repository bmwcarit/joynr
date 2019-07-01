module.exports = {
    transform: {
        "\\.ts$": "ts-jest"
    },
    testEnvironment: "node",
    testRegex: "src/test/js/node_integration/consumer/.*Test\\.ts$",
    moduleFileExtensions: ["ts", "js", "json", "node"],
    modulePathIgnorePatterns: ["target"]
};
