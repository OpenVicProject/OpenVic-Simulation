#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/testing/TestScript.hpp"

#include "utility/Containers.hpp"

namespace OpenVic {
	class A_001_file_tests : public TestScript {

	public:
		A_001_file_tests() : TestScript { "A_001_file_tests" } {
			add_requirements();
		}

		void add_requirements() {
			memory::unique_ptr<Requirement> FS_44 = memory::make_unique<Requirement>(
				"FS_44",
				"The icon for the Canned Food good shall be loaded from the R/art/economy/goods folder with the filename "
				"Canned Food.png",
				"The icon for the Canned Food good has been loaded into the program"
			);
			add_requirement(std::move(FS_44));
			memory::unique_ptr<Requirement> FS_48 = memory::make_unique<Requirement>(
				"FS_48",
				"The icon for the Coal good shall be loaded from the R/art/economy/goods folder with the filename Coal.png",
				"The icon for the Coal good has been loaded into the program"
			);
			add_requirement(std::move(FS_48));
			memory::unique_ptr<Requirement> FS_61 = memory::make_unique<Requirement>(
				"FS_61",
				"The icon for the Grain good shall be loaded from the R/art/economy/goods folder with the filename Grain.png",
				"The icon for the Grain good has been loaded into the program"
			);
			add_requirement(std::move(FS_61));
			memory::unique_ptr<Requirement> FS_62 = memory::make_unique<Requirement>(
				"FS_62",
				"The icon for the Iron good shall be loaded from the R/art/economy/goods folder with the filename Iron.png",
				"The icon for the Iron good has been loaded into the program"
			);
			add_requirement(std::move(FS_62));
			memory::unique_ptr<Requirement> FS_63 = memory::make_unique<Requirement>(
				"FS_63",
				"The icon for the Liquor good shall be loaded from the R/art/economy/goods folder with the filename Liquor.png",
				"The icon for the Liquor good has been loaded into the program"
			);
			add_requirement(std::move(FS_63));
			memory::unique_ptr<Requirement> FS_67 = memory::make_unique<Requirement>(
				"FS_67",
				"The icon for the Machine Parts good shall be loaded from the R/art/economy/goods folder with the filename "
				"Machine Parts.png",
				"The icon for the Machine Parts good has been loaded into the program"
			);
			add_requirement(std::move(FS_67));
			memory::unique_ptr<Requirement> FS_86 = memory::make_unique<Requirement>(
				"FS_86",
				"The icon for the Wool good shall be loaded from the R/art/economy/goods folder with the filename Wool.png",
				"The icon for the Wool good has been loaded into the program"
			);
			add_requirement(std::move(FS_86));
			memory::unique_ptr<Requirement> FS_2 = memory::make_unique<Requirement>(
				"FS_2", "User provided data shall be saved to an 'OpenVic' folder, located following platform convention",
				"User data is saved to the correct place"
			);
			add_requirement(std::move(FS_2));
			memory::unique_ptr<Requirement> FS_20 = memory::make_unique<Requirement>(
				"FS_20", "On Windows, user provided data shall be saved by default to '%APPDATA%/OpenVic/'",
				"User data is saved to the correct place"
			);
			add_requirement(std::move(FS_20));
			memory::unique_ptr<Requirement> FS_21 = memory::make_unique<Requirement>(
				"FS_21", "On Linux, user provided data shall be saved by default to '~/.local/share/OpenVic/'",
				"User data is saved to the correct place"
			);
			add_requirement(std::move(FS_21));
			memory::unique_ptr<Requirement> FS_22 = memory::make_unique<Requirement>(
				"FS_22", "On macOS, user provided data shall be saved by default to '~/Library/Application Support/OpenVic/'",
				"User data is saved to the correct place"
			);
			add_requirement(std::move(FS_22));
			memory::unique_ptr<Requirement> FS_24 = memory::make_unique<Requirement>(
				"FS_24", "All .csv files in the locale folder shall contain translation keys and translations",
				"No errant files in locale directory"
			);
			add_requirement(std::move(FS_24));
			memory::unique_ptr<Requirement> FS_17 = memory::make_unique<Requirement>(
				"FS_17", "List of available locales are loaded from R/localisation/ directory", "Locales loaded correctly"
			);
			add_requirement(std::move(FS_17));
			memory::unique_ptr<Requirement> FS_333 = memory::make_unique<Requirement>(
				"FS_333", "The map's provinces shall be defined by unique colours in 'R/map/provinces.bmp'",
				"The unique colours of 'R/map/provinces.bmp' define provinces"
			);
			add_requirement(std::move(FS_333));
			memory::unique_ptr<Requirement> FS_335 = memory::make_unique<Requirement>(
				"FS_335", "Unique province IDs shall be associated with their unique colours in 'R/map/definition.csv'",
				"'R/map/definition.csv' associates every unique colour used to define a province with a unique ID"
			);
			add_requirement(std::move(FS_335));
			memory::unique_ptr<Requirement> FS_334 = memory::make_unique<Requirement>(
				"FS_334", "Water provinces shall be defined by a list of their IDs in 'R/map/default.map'",
				"'R/map/default.map' contains a list of province IDs which are used to define water provinces"
			);
			add_requirement(std::move(FS_334));
			memory::unique_ptr<Requirement> FS_338 = memory::make_unique<Requirement>(
				"FS_338",
				"The image for the minimap background shall be loaded from the R/art/ui folder with the filename minimap.png",
				"The image for the minimap background has been loaded into the program"
			);
			add_requirement(std::move(FS_338));
			memory::unique_ptr<Requirement> FS_343 = memory::make_unique<Requirement>(
				"FS_343", "The textures making up the cosmetic terrain map shall be loaded from the R/art/terrain folder",
				"The textures making up the cosmetic terrain map have been loaded into the program"
			);
			add_requirement(std::move(FS_343));
			memory::unique_ptr<Requirement> FS_341 = memory::make_unique<Requirement>(
				"FS_341", "State areas shall be defined by lists of province IDs in 'R/map/region.txt'",
				"'R/map/region.txt' defines state areas with lists of province IDs"
			);
			add_requirement(std::move(FS_341));
			memory::unique_ptr<Requirement> SND_10 = memory::make_unique<Requirement>(
				"SND_10", "SFX shall be referred to by their filename, without the extension",
				"Sound effects are identified by their filename without extension"
			);
			add_requirement(std::move(SND_10));
		}

		void execute_script() {
			// FS_44
			// The icon for the Canned Food good shall be loaded from the R/art/economy/goods folder with the filename Canned
			// Food.png The icon for the Canned Food good has been loaded into the program

			// TODO: Write test steps for FS_44...

			// FS_48
			// The icon for the Coal good shall be loaded from the R/art/economy/goods folder with the filename Coal.png
			// The icon for the Coal good has been loaded into the program

			// TODO: Write test steps for FS_48...

			// FS_61
			// The icon for the Grain good shall be loaded from the R/art/economy/goods folder with the filename Grain.png
			// The icon for the Grain good has been loaded into the program

			// TODO: Write test steps for FS_61...

			// FS_62
			// The icon for the Iron good shall be loaded from the R/art/economy/goods folder with the filename Iron.png
			// The icon for the Iron good has been loaded into the program

			// TODO: Write test steps for FS_62...

			// FS_63
			// The icon for the Liquor good shall be loaded from the R/art/economy/goods folder with the filename Liquor.png
			// The icon for the Liquor good has been loaded into the program

			// TODO: Write test steps for FS_63...

			// FS_67
			// The icon for the Machine Parts good shall be loaded from the R/art/economy/goods folder with the filename
			// Machine Parts.png The icon for the Machine Parts good has been loaded into the program

			// TODO: Write test steps for FS_67...

			// FS_86
			// The icon for the Wool good shall be loaded from the R/art/economy/goods folder with the filename Wool.png
			// The icon for the Wool good has been loaded into the program

			// TODO: Write test steps for FS_86...

			// FS_2
			// User provided data shall be saved to an 'OpenVic' folder, located following platform convention
			// User data is saved to the correct place

			// TODO: Write test steps for FS_2...

			// FS_20
			// On Windows, user provided data shall be saved by default to '%APPDATA%/OpenVic/'
			// User data is saved to the correct place

			// TODO: Write test steps for FS_20...

			// FS_21
			// On Linux, user provided data shall be saved by default to '~/.local/share/OpenVic/'
			// User data is saved to the correct place

			// TODO: Write test steps for FS_21...

			// FS_22
			// On macOS, user provided data shall be saved by default to '~/Library/Application Support/OpenVic/'
			// User data is saved to the correct place

			// TODO: Write test steps for FS_22...

			// FS_24
			// All .csv files in the locale folder shall contain translation keys and translations
			// No errant files in locale directory

			// TODO: Write test steps for FS_24...

			// FS_17
			// List of available locales are loaded from R/localisation/ directory
			// Locales loaded correctly

			// TODO: Write test steps for FS_17...

			// FS_333
			// The map's provinces shall be defined by unique colours in 'R/map/provinces.bmp'
			// The unique colours of 'R/map/provinces.bmp' define provinces

			// TODO: Write test steps for FS_333...

			// FS_335
			// Unique province IDs shall be associated with their unique colours in 'R/map/definition.csv'
			// 'R/map/definition.csv' associates every unique colour used to define a province with a unique ID

			// TODO: Write test steps for FS_335...

			// FS_334
			// Water provinces shall be defined by a list of their IDs in 'R/map/default.map'
			// 'R/map/default.map' contains a list of province IDs which are used to define water provinces

			// TODO: Write test steps for FS_334...

			// FS_338
			// The image for the minimap background shall be loaded from the R/art/ui folder with the filename minimap.png
			// The image for the minimap background has been loaded into the program

			// TODO: Write test steps for FS_338...

			// FS_343
			// The textures making up the cosmetic terrain map shall be loaded from the R/art/terrain folder
			// The textures making up the cosmetic terrain map have been loaded into the program

			// TODO: Write test steps for FS_343...

			// FS_341
			// State areas shall be defined by lists of province IDs in 'R/map/region.txt'
			// 'R/map/region.txt' defines state areas with lists of province IDs

			// TODO: Write test steps for FS_341...

			// SND_10
			// SFX shall be referred to by their filename, without the extension
			// Sound effects are identified by their filename without extension

			// TODO: Write test steps for SND_10...
		}
	};
}
