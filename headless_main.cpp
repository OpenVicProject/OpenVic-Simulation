#ifdef OPENVIC_HEADLESS_SIM
#include <iostream>
#include <string>
#include "src/openvic/Simulation.hpp"
#include "src/openvic/dataloader/Dataloader.hpp"


int main() {
	std::cout << "HEADLESS SIMULATION" << std::endl;


	std::string vic2FolderLocation = "";
	if (vic2FolderLocation.length() <= 0) {
		std::cout << "Path to Victoria 2 folder not specified. Manually specify location: ";
		std::cin >> vic2FolderLocation;
	}
	std::filesystem::path path(vic2FolderLocation);

	OpenVic::Simulation sim;
	std::cout << (OpenVic::Dataloader::loadDir(path, sim) ? "Dataloader suceeded" : "Dataloader failed") << std::endl;
}
#endif