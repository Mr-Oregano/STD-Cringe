#include "utils//misc//cringe_environment.h"

auto get_env(const std::string_view &given_key) -> std::string {
	std::string line;
	std::string key;
	std::string value;
	std::ifstream file(".env");

	// Open the .env file and error if can't open
	if (!file.is_open()) {
		std::cerr << "Error opening file." << '\n';
		return "";
	}

	// Read and parse the file line by line
	while (std::getline(file, line)) {
		// Find the position of the equal sign
		size_t equalPos = line.find('=');

		// Check if the line contains an equal sign and it is not the first character
		if (equalPos != std::string::npos && equalPos > 0) {
			// Extract key and value based on the position of the equal sign
			key = line.substr(0, equalPos);
			value = line.substr(equalPos + 1);

			// Trim leading and trailing whitespaces from key and value
			key.erase(0, key.find_first_not_of(" \t"));
			key.erase(key.find_last_not_of(" \t") + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);

			// Check to see if the key matches what we're looking for
			if (key == given_key) {
				// Close the file
				file.close();
				return value;
			}
		}
	}

	// Close the file
	file.close();

	// No match
	return "";
}
