# pragma once
# include <testing/TestScript.hpp>

namespace OpenVic {
	class A_002_economy_tests : public TestScript {

	public:
		A_002_economy_tests() {
			add_requirements();
			execute_script();
		}

		void add_requirements() {
			Requirement ECON_123 = Requirement("ECON_123",
 				"The base price for Aeroplanes shall be 110",
 				"The base price of 110 for Aeroplanes can be seen in program output data");
			add_requirement(ECON_123);
			Requirement ECON_124 = Requirement("ECON_124",
 				"The base price for Ammunition shall be 17.5",
 				"The base price of 17.5 for Ammunition can be seen in program output data");
			add_requirement(ECON_124);
			Requirement ECON_125 = Requirement("ECON_125",
 				"The base price for Artillery shall be 60",
 				"The base price of 60 for Artillery can be seen in program output data");
			add_requirement(ECON_125);
			Requirement ECON_126 = Requirement("ECON_126",
 				"The base price for Automobiles shall be 70",
 				"The base price of 70 for Automobiles can be seen in program output data");
			add_requirement(ECON_126);
			Requirement ECON_127 = Requirement("ECON_127",
 				"The base price for Canned Food shall be 16",
 				"The base price of 16 for Canned Food can be seen in program output data");
			add_requirement(ECON_127);
			Requirement ECON_128 = Requirement("ECON_128",
 				"The base price for Cattle shall be 2",
 				"The base price of 2 for Cattle can be seen in program output data");
			add_requirement(ECON_128);
			Requirement ECON_129 = Requirement("ECON_129",
 				"The base price for Cement shall be 16",
 				"The base price of 16 for Cement can be seen in program output data");
			add_requirement(ECON_129);
			Requirement ECON_130 = Requirement("ECON_130",
 				"The base price for Clipper Convoys shall be 42",
 				"The base price of 42 for Clipper Convoys can be seen in program output data");
			add_requirement(ECON_130);
			Requirement ECON_131 = Requirement("ECON_131",
 				"The base price for Coal shall be 2.3",
 				"The base price of 2.3 for Coal can be seen in program output data");
			add_requirement(ECON_131);
			Requirement ECON_132 = Requirement("ECON_132",
 				"The base price for Coffee shall be 2.1",
 				"The base price of 2.1 for Coffee can be seen in program output data");
			add_requirement(ECON_132);
			Requirement ECON_133 = Requirement("ECON_133",
 				"The base price for Cotton shall be 2",
 				"The base price of 2 for Cotton can be seen in program output data");
			add_requirement(ECON_133);
			Requirement ECON_134 = Requirement("ECON_134",
 				"The base price for Dye shall be 12",
 				"The base price of 12 for Dye can be seen in program output data");
			add_requirement(ECON_134);
			Requirement ECON_135 = Requirement("ECON_135",
 				"The base price for Electric Gear shall be 16",
 				"The base price of 16 for Electric Gear can be seen in program output data");
			add_requirement(ECON_135);
			Requirement ECON_136 = Requirement("ECON_136",
 				"The base price for Explosives shall be 20",
 				"The base price of 20 for Explosives can be seen in program output data");
			add_requirement(ECON_136);
			Requirement ECON_137 = Requirement("ECON_137",
 				"The base price for Fabric shall be 1.8",
 				"The base price of 1.8 for Fabric can be seen in program output data");
			add_requirement(ECON_137);
			Requirement ECON_138 = Requirement("ECON_138",
 				"The base price for Fertilizer shall be 10",
 				"The base price of 10 for Fertilizer can be seen in program output data");
			add_requirement(ECON_138);
			Requirement ECON_139 = Requirement("ECON_139",
 				"The base price for Fish shall be 1.5",
 				"The base price of 1.5 for Fish can be seen in program output data");
			add_requirement(ECON_139);
			Requirement ECON_140 = Requirement("ECON_140",
 				"The base price for Fruit shall be 1.8",
 				"The base price of 1.8 for Fruit can be seen in program output data");
			add_requirement(ECON_140);
			Requirement ECON_141 = Requirement("ECON_141",
 				"The base price for Fuel shall be 25",
 				"The base price of 25 for Fuel can be seen in program output data");
			add_requirement(ECON_141);
			Requirement ECON_142 = Requirement("ECON_142",
 				"The base price for Furniture shall be 4.9",
 				"The base price of 4.9 for Furniture can be seen in program output data");
			add_requirement(ECON_142);
			Requirement ECON_234 = Requirement("ECON_234",
 				"The base price for Glass shall be 2.9",
 				"The base price of 2.9 for Glass can be seen in program output data");
			add_requirement(ECON_234);
			Requirement ECON_235 = Requirement("ECON_235",
 				"The base price for Grain shall be 2.2",
 				"The base price of 2.2 for Grain can be seen in program output data");
			add_requirement(ECON_235);
			Requirement ECON_236 = Requirement("ECON_236",
 				"The base price for Iron shall be 3.5",
 				"The base price of 3.5 for Iron can be seen in program output data");
			add_requirement(ECON_236);
			Requirement ECON_237 = Requirement("ECON_237",
 				"The base price for Liquor shall be 6.4",
 				"The base price of 6.4 for Liquor can be seen in program output data");
			add_requirement(ECON_237);
			Requirement ECON_238 = Requirement("ECON_238",
 				"The base price for Lumber shall be 1",
 				"The base price of 1 for Lumber can be seen in program output data");
			add_requirement(ECON_238);
			Requirement ECON_239 = Requirement("ECON_239",
 				"The base price for Luxury Clothes shall be 65",
 				"The base price of 65 for Luxury Clothes can be seen in program output data");
			add_requirement(ECON_239);
			Requirement ECON_240 = Requirement("ECON_240",
 				"The base price for Luxury Furniture shall be 59",
 				"The base price of 59 for Luxury Furniture can be seen in program output data");
			add_requirement(ECON_240);
			Requirement ECON_241 = Requirement("ECON_241",
 				"The base price for Machine Parts shall be 36.5",
 				"The base price of 36.5 for Machine Parts can be seen in program output data");
			add_requirement(ECON_241);
			Requirement ECON_242 = Requirement("ECON_242",
 				"The base price for Oil shall be 12",
 				"The base price of 12 for Oil can be seen in program output data");
			add_requirement(ECON_242);
			Requirement ECON_243 = Requirement("ECON_243",
 				"The base price for Opium shall be 3.2",
 				"The base price of 3.2 for Opium can be seen in program output data");
			add_requirement(ECON_243);
			Requirement ECON_244 = Requirement("ECON_244",
 				"The base price for Paper shall be 3.4",
 				"The base price of 3.4 for Paper can be seen in program output data");
			add_requirement(ECON_244);
			Requirement ECON_245 = Requirement("ECON_245",
 				"The base price for Precious Metal  shall be 8",
 				"The base price of 8 for Precious Metal can be seen in program output data");
			add_requirement(ECON_245);
			Requirement ECON_246 = Requirement("ECON_246",
 				"The base price for Radios shall be 16",
 				"The base price of 16 for Radios can be seen in program output data");
			add_requirement(ECON_246);
			Requirement ECON_247 = Requirement("ECON_247",
 				"The base price for Regular Clothes shall be 5.8",
 				"The base price of 5.8 for Regular Clothes can be seen in program output data");
			add_requirement(ECON_247);
			Requirement ECON_248 = Requirement("ECON_248",
 				"The base price for Rubber shall be 7",
 				"The base price of 7 for Rubber can be seen in program output data");
			add_requirement(ECON_248);
			Requirement ECON_249 = Requirement("ECON_249",
 				"The base price for Silk shall be 10",
 				"The base price of 10 for Silk can be seen in program output data");
			add_requirement(ECON_249);
			Requirement ECON_250 = Requirement("ECON_250",
 				"The base price for Small Arms shall be 37",
 				"The base price of 37 for Small Arms can be seen in program output data");
			add_requirement(ECON_250);
			Requirement ECON_251 = Requirement("ECON_251",
 				"The base price for Steamer Convoys shall be 65",
 				"The base price of 65 for Steamer Convoys can be seen in program output data");
			add_requirement(ECON_251);
			Requirement ECON_252 = Requirement("ECON_252",
 				"The base price for Steel shall be 4.7",
 				"The base price of 4.7 for Steel can be seen in program output data");
			add_requirement(ECON_252);
			Requirement ECON_253 = Requirement("ECON_253",
 				"The base price for Sulphur shall be 6",
 				"The base price of 6 for Sulphur can be seen in program output data");
			add_requirement(ECON_253);
			Requirement ECON_254 = Requirement("ECON_254",
 				"The base price for Tanks shall be 98",
 				"The base price of 98 for Tanks can be seen in program output data");
			add_requirement(ECON_254);
			Requirement ECON_255 = Requirement("ECON_255",
 				"The base price for Tea shall be 2.6",
 				"The base price of 2.6 for Tea can be seen in program output data");
			add_requirement(ECON_255);
			Requirement ECON_256 = Requirement("ECON_256",
 				"The base price for Telephones shall be 16",
 				"The base price of 16 for Telephones can be seen in program output data");
			add_requirement(ECON_256);
			Requirement ECON_257 = Requirement("ECON_257",
 				"The base price for Timber shall be 0.9",
 				"The base price of 0.9 for Timber can be seen in program output data");
			add_requirement(ECON_257);
			Requirement ECON_258 = Requirement("ECON_258",
 				"The base price for Tobacco shall be 1.1",
 				"The base price of 1.1 for Tobacco can be seen in program output data");
			add_requirement(ECON_258);
			Requirement ECON_259 = Requirement("ECON_259",
 				"The base price for Tropical Wood shall be 5.4",
 				"The base price of 5.4 for Tropical Wood can be seen in program output data");
			add_requirement(ECON_259);
			Requirement ECON_260 = Requirement("ECON_260",
 				"The base price for Wine shall be 9.7",
 				"The base price of 9.7 for Wine can be seen in program output data");
			add_requirement(ECON_260);
			Requirement ECON_261 = Requirement("ECON_261",
 				"The base price for Wool shall be 0.7",
 				"The base price of 0.7 for Wool can be seen in program output data");
			add_requirement(ECON_261);
		}

		void execute_script() {
			// ECON_123
			// The base price for Aeroplanes shall be 110
			// The base price of 110 for Aeroplanes can be seen in program output data

			// TODO: Write test steps for ECON_123...

			// ECON_124
			// The base price for Ammunition shall be 17.5
			// The base price of 17.5 for Ammunition can be seen in program output data

			// TODO: Write test steps for ECON_124...

			// ECON_125
			// The base price for Artillery shall be 60
			// The base price of 60 for Artillery can be seen in program output data

			// TODO: Write test steps for ECON_125...

			// ECON_126
			// The base price for Automobiles shall be 70
			// The base price of 70 for Automobiles can be seen in program output data

			// TODO: Write test steps for ECON_126...

			// ECON_127
			// The base price for Canned Food shall be 16
			// The base price of 16 for Canned Food can be seen in program output data

			// TODO: Write test steps for ECON_127...

			// ECON_128
			// The base price for Cattle shall be 2
			// The base price of 2 for Cattle can be seen in program output data

			// TODO: Write test steps for ECON_128...

			// ECON_129
			// The base price for Cement shall be 16
			// The base price of 16 for Cement can be seen in program output data

			// TODO: Write test steps for ECON_129...

			// ECON_130
			// The base price for Clipper Convoys shall be 42
			// The base price of 42 for Clipper Convoys can be seen in program output data

			// TODO: Write test steps for ECON_130...

			// ECON_131
			// The base price for Coal shall be 2.3
			// The base price of 2.3 for Coal can be seen in program output data

			// TODO: Write test steps for ECON_131...

			// ECON_132
			// The base price for Coffee shall be 2.1
			// The base price of 2.1 for Coffee can be seen in program output data

			// TODO: Write test steps for ECON_132...

			// ECON_133
			// The base price for Cotton shall be 2
			// The base price of 2 for Cotton can be seen in program output data

			// TODO: Write test steps for ECON_133...

			// ECON_134
			// The base price for Dye shall be 12
			// The base price of 12 for Dye can be seen in program output data

			// TODO: Write test steps for ECON_134...

			// ECON_135
			// The base price for Electric Gear shall be 16
			// The base price of 16 for Electric Gear can be seen in program output data

			// TODO: Write test steps for ECON_135...

			// ECON_136
			// The base price for Explosives shall be 20
			// The base price of 20 for Explosives can be seen in program output data

			// TODO: Write test steps for ECON_136...

			// ECON_137
			// The base price for Fabric shall be 1.8
			// The base price of 1.8 for Fabric can be seen in program output data

			// TODO: Write test steps for ECON_137...

			// ECON_138
			// The base price for Fertilizer shall be 10
			// The base price of 10 for Fertilizer can be seen in program output data

			// TODO: Write test steps for ECON_138...

			// ECON_139
			// The base price for Fish shall be 1.5
			// The base price of 1.5 for Fish can be seen in program output data

			// TODO: Write test steps for ECON_139...

			// ECON_140
			// The base price for Fruit shall be 1.8
			// The base price of 1.8 for Fruit can be seen in program output data

			// TODO: Write test steps for ECON_140...

			// ECON_141
			// The base price for Fuel shall be 25
			// The base price of 25 for Fuel can be seen in program output data

			// TODO: Write test steps for ECON_141...

			// ECON_142
			// The base price for Furniture shall be 4.9
			// The base price of 4.9 for Furniture can be seen in program output data

			// TODO: Write test steps for ECON_142...

			// ECON_234
			// The base price for Glass shall be 2.9
			// The base price of 2.9 for Glass can be seen in program output data

			// TODO: Write test steps for ECON_234...

			// ECON_235
			// The base price for Grain shall be 2.2
			// The base price of 2.2 for Grain can be seen in program output data

			// TODO: Write test steps for ECON_235...

			// ECON_236
			// The base price for Iron shall be 3.5
			// The base price of 3.5 for Iron can be seen in program output data

			// TODO: Write test steps for ECON_236...

			// ECON_237
			// The base price for Liquor shall be 6.4
			// The base price of 6.4 for Liquor can be seen in program output data

			// TODO: Write test steps for ECON_237...

			// ECON_238
			// The base price for Lumber shall be 1
			// The base price of 1 for Lumber can be seen in program output data

			// TODO: Write test steps for ECON_238...

			// ECON_239
			// The base price for Luxury Clothes shall be 65
			// The base price of 65 for Luxury Clothes can be seen in program output data

			// TODO: Write test steps for ECON_239...

			// ECON_240
			// The base price for Luxury Furniture shall be 59
			// The base price of 59 for Luxury Furniture can be seen in program output data

			// TODO: Write test steps for ECON_240...

			// ECON_241
			// The base price for Machine Parts shall be 36.5
			// The base price of 36.5 for Machine Parts can be seen in program output data

			// TODO: Write test steps for ECON_241...

			// ECON_242
			// The base price for Oil shall be 12
			// The base price of 12 for Oil can be seen in program output data

			// TODO: Write test steps for ECON_242...

			// ECON_243
			// The base price for Opium shall be 3.2
			// The base price of 3.2 for Opium can be seen in program output data

			// TODO: Write test steps for ECON_243...

			// ECON_244
			// The base price for Paper shall be 3.4
			// The base price of 3.4 for Paper can be seen in program output data

			// TODO: Write test steps for ECON_244...

			// ECON_245
			// The base price for Precious Metal  shall be 8
			// The base price of 8 for Precious Metal can be seen in program output data

			// TODO: Write test steps for ECON_245...

			// ECON_246
			// The base price for Radios shall be 16
			// The base price of 16 for Radios can be seen in program output data

			// TODO: Write test steps for ECON_246...

			// ECON_247
			// The base price for Regular Clothes shall be 5.8
			// The base price of 5.8 for Regular Clothes can be seen in program output data

			// TODO: Write test steps for ECON_247...

			// ECON_248
			// The base price for Rubber shall be 7
			// The base price of 7 for Rubber can be seen in program output data

			// TODO: Write test steps for ECON_248...

			// ECON_249
			// The base price for Silk shall be 10
			// The base price of 10 for Silk can be seen in program output data

			// TODO: Write test steps for ECON_249...

			// ECON_250
			// The base price for Small Arms shall be 37
			// The base price of 37 for Small Arms can be seen in program output data

			// TODO: Write test steps for ECON_250...

			// ECON_251
			// The base price for Steamer Convoys shall be 65
			// The base price of 65 for Steamer Convoys can be seen in program output data

			// TODO: Write test steps for ECON_251...

			// ECON_252
			// The base price for Steel shall be 4.7
			// The base price of 4.7 for Steel can be seen in program output data

			// TODO: Write test steps for ECON_252...

			// ECON_253
			// The base price for Sulphur shall be 6
			// The base price of 6 for Sulphur can be seen in program output data

			// TODO: Write test steps for ECON_253...

			// ECON_254
			// The base price for Tanks shall be 98
			// The base price of 98 for Tanks can be seen in program output data

			// TODO: Write test steps for ECON_254...

			// ECON_255
			// The base price for Tea shall be 2.6
			// The base price of 2.6 for Tea can be seen in program output data

			// TODO: Write test steps for ECON_255...

			// ECON_256
			// The base price for Telephones shall be 16
			// The base price of 16 for Telephones can be seen in program output data

			// TODO: Write test steps for ECON_256...

			// ECON_257
			// The base price for Timber shall be 0.9
			// The base price of 0.9 for Timber can be seen in program output data

			// TODO: Write test steps for ECON_257...

			// ECON_258
			// The base price for Tobacco shall be 1.1
			// The base price of 1.1 for Tobacco can be seen in program output data

			// TODO: Write test steps for ECON_258...

			// ECON_259
			// The base price for Tropical Wood shall be 5.4
			// The base price of 5.4 for Tropical Wood can be seen in program output data

			// TODO: Write test steps for ECON_259...

			// ECON_260
			// The base price for Wine shall be 9.7
			// The base price of 9.7 for Wine can be seen in program output data

			// TODO: Write test steps for ECON_260...

			// ECON_261
			// The base price for Wool shall be 0.7
			// The base price of 0.7 for Wool can be seen in program output data

			// TODO: Write test steps for ECON_261...

		}
	};
}
