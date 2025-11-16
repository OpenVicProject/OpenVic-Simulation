#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/testing/TestScript.hpp"

namespace OpenVic {
	class A_002_economy_tests : public TestScript {

	public:
		A_002_economy_tests() : TestScript { "A_002_economy_tests" } {
			add_requirements();
		}

		void add_requirements() {
			memory::unique_ptr<Requirement> ECON_123 = memory::make_unique<Requirement>(
				"ECON_123", "The base price for Aeroplanes shall be 110",
				"The base price of 110 for Aeroplanes can be seen in program output data"
			);
			add_requirement(std::move(ECON_123));
			memory::unique_ptr<Requirement> ECON_124 = memory::make_unique<Requirement>(
				"ECON_124", "The base price for Ammunition shall be 17.5",
				"The base price of 17.5 for Ammunition can be seen in program output data"
			);
			add_requirement(std::move(ECON_124));
			memory::unique_ptr<Requirement> ECON_125 = memory::make_unique<Requirement>(
				"ECON_125", "The base price for Artillery shall be 60",
				"The base price of 60 for Artillery can be seen in program output data"
			);
			add_requirement(std::move(ECON_125));
			memory::unique_ptr<Requirement> ECON_126 = memory::make_unique<Requirement>(
				"ECON_126", "The base price for Automobiles shall be 70",
				"The base price of 70 for Automobiles can be seen in program output data"
			);
			add_requirement(std::move(ECON_126));
			memory::unique_ptr<Requirement> ECON_127 = memory::make_unique<Requirement>(
				"ECON_127", "The base price for Canned Food shall be 16",
				"The base price of 16 for Canned Food can be seen in program output data"
			);
			add_requirement(std::move(ECON_127));
			memory::unique_ptr<Requirement> ECON_128 = memory::make_unique<Requirement>(
				"ECON_128", "The base price for Cattle shall be 2",
				"The base price of 2 for Cattle can be seen in program output data"
			);
			add_requirement(std::move(ECON_128));
			memory::unique_ptr<Requirement> ECON_129 = memory::make_unique<Requirement>(
				"ECON_129", "The base price for Cement shall be 16",
				"The base price of 16 for Cement can be seen in program output data"
			);
			add_requirement(std::move(ECON_129));
			memory::unique_ptr<Requirement> ECON_130 = memory::make_unique<Requirement>(
				"ECON_130", "The base price for Clipper Convoys shall be 42",
				"The base price of 42 for Clipper Convoys can be seen in program output data"
			);
			add_requirement(std::move(ECON_130));
			memory::unique_ptr<Requirement> ECON_131 = memory::make_unique<Requirement>(
				"ECON_131", "The base price for Coal shall be 2.3",
				"The base price of 2.3 for Coal can be seen in program output data"
			);
			add_requirement(std::move(ECON_131));
			memory::unique_ptr<Requirement> ECON_132 = memory::make_unique<Requirement>(
				"ECON_132", "The base price for Coffee shall be 2.1",
				"The base price of 2.1 for Coffee can be seen in program output data"
			);
			add_requirement(std::move(ECON_132));
			memory::unique_ptr<Requirement> ECON_133 = memory::make_unique<Requirement>(
				"ECON_133", "The base price for Cotton shall be 2",
				"The base price of 2 for Cotton can be seen in program output data"
			);
			add_requirement(std::move(ECON_133));
			memory::unique_ptr<Requirement> ECON_134 = memory::make_unique<Requirement>(
				"ECON_134", "The base price for Dye shall be 12",
				"The base price of 12 for Dye can be seen in program output data"
			);
			add_requirement(std::move(ECON_134));
			memory::unique_ptr<Requirement> ECON_135 = memory::make_unique<Requirement>(
				"ECON_135", "The base price for Electric Gear shall be 16",
				"The base price of 16 for Electric Gear can be seen in program output data"
			);
			add_requirement(std::move(ECON_135));
			memory::unique_ptr<Requirement> ECON_136 = memory::make_unique<Requirement>(
				"ECON_136", "The base price for Explosives shall be 20",
				"The base price of 20 for Explosives can be seen in program output data"
			);
			add_requirement(std::move(ECON_136));
			memory::unique_ptr<Requirement> ECON_137 = memory::make_unique<Requirement>(
				"ECON_137", "The base price for Fabric shall be 1.8",
				"The base price of 1.8 for Fabric can be seen in program output data"
			);
			add_requirement(std::move(ECON_137));
			memory::unique_ptr<Requirement> ECON_138 = memory::make_unique<Requirement>(
				"ECON_138", "The base price for Fertilizer shall be 10",
				"The base price of 10 for Fertilizer can be seen in program output data"
			);
			add_requirement(std::move(ECON_138));
			memory::unique_ptr<Requirement> ECON_139 = memory::make_unique<Requirement>(
				"ECON_139", "The base price for Fish shall be 1.5",
				"The base price of 1.5 for Fish can be seen in program output data"
			);
			add_requirement(std::move(ECON_139));
			memory::unique_ptr<Requirement> ECON_140 = memory::make_unique<Requirement>(
				"ECON_140", "The base price for Fruit shall be 1.8",
				"The base price of 1.8 for Fruit can be seen in program output data"
			);
			add_requirement(std::move(ECON_140));
			memory::unique_ptr<Requirement> ECON_141 = memory::make_unique<Requirement>(
				"ECON_141", "The base price for Fuel shall be 25",
				"The base price of 25 for Fuel can be seen in program output data"
			);
			add_requirement(std::move(ECON_141));
			memory::unique_ptr<Requirement> ECON_142 = memory::make_unique<Requirement>(
				"ECON_142", "The base price for Furniture shall be 4.9",
				"The base price of 4.9 for Furniture can be seen in program output data"
			);
			add_requirement(std::move(ECON_142));
			memory::unique_ptr<Requirement> ECON_234 = memory::make_unique<Requirement>(
				"ECON_234", "The base price for Glass shall be 2.9",
				"The base price of 2.9 for Glass can be seen in program output data"
			);
			add_requirement(std::move(ECON_234));
			memory::unique_ptr<Requirement> ECON_235 = memory::make_unique<Requirement>(
				"ECON_235", "The base price for Grain shall be 2.2",
				"The base price of 2.2 for Grain can be seen in program output data"
			);
			add_requirement(std::move(ECON_235));
			memory::unique_ptr<Requirement> ECON_236 = memory::make_unique<Requirement>(
				"ECON_236", "The base price for Iron shall be 3.5",
				"The base price of 3.5 for Iron can be seen in program output data"
			);
			add_requirement(std::move(ECON_236));
			memory::unique_ptr<Requirement> ECON_237 = memory::make_unique<Requirement>(
				"ECON_237", "The base price for Liquor shall be 6.4",
				"The base price of 6.4 for Liquor can be seen in program output data"
			);
			add_requirement(std::move(ECON_237));
			memory::unique_ptr<Requirement> ECON_238 = memory::make_unique<Requirement>(
				"ECON_238", "The base price for Lumber shall be 1",
				"The base price of 1 for Lumber can be seen in program output data"
			);
			add_requirement(std::move(ECON_238));
			memory::unique_ptr<Requirement> ECON_239 = memory::make_unique<Requirement>(
				"ECON_239", "The base price for Luxury Clothes shall be 65",
				"The base price of 65 for Luxury Clothes can be seen in program output data"
			);
			add_requirement(std::move(ECON_239));
			memory::unique_ptr<Requirement> ECON_240 = memory::make_unique<Requirement>(
				"ECON_240", "The base price for Luxury Furniture shall be 59",
				"The base price of 59 for Luxury Furniture can be seen in program output data"
			);
			add_requirement(std::move(ECON_240));
			memory::unique_ptr<Requirement> ECON_241 = memory::make_unique<Requirement>(
				"ECON_241", "The base price for Machine Parts shall be 36.5",
				"The base price of 36.5 for Machine Parts can be seen in program output data"
			);
			add_requirement(std::move(ECON_241));
			memory::unique_ptr<Requirement> ECON_242 = memory::make_unique<Requirement>(
				"ECON_242", "The base price for Oil shall be 12",
				"The base price of 12 for Oil can be seen in program output data"
			);
			add_requirement(std::move(ECON_242));
			memory::unique_ptr<Requirement> ECON_243 = memory::make_unique<Requirement>(
				"ECON_243", "The base price for Opium shall be 3.2",
				"The base price of 3.2 for Opium can be seen in program output data"
			);
			add_requirement(std::move(ECON_243));
			memory::unique_ptr<Requirement> ECON_244 = memory::make_unique<Requirement>(
				"ECON_244", "The base price for Paper shall be 3.4",
				"The base price of 3.4 for Paper can be seen in program output data"
			);
			add_requirement(std::move(ECON_244));
			memory::unique_ptr<Requirement> ECON_245 = memory::make_unique<Requirement>(
				"ECON_245", "The base price for Precious Metal shall be 8",
				"The base price of 8 for Precious Metal can be seen in program output data"
			);
			add_requirement(std::move(ECON_245));
			memory::unique_ptr<Requirement> ECON_246 = memory::make_unique<Requirement>(
				"ECON_246", "The base price for Radios shall be 16",
				"The base price of 16 for Radios can be seen in program output data"
			);
			add_requirement(std::move(ECON_246));
			memory::unique_ptr<Requirement> ECON_247 = memory::make_unique<Requirement>(
				"ECON_247", "The base price for Regular Clothes shall be 5.8",
				"The base price of 5.8 for Regular Clothes can be seen in program output data"
			);
			add_requirement(std::move(ECON_247));
			memory::unique_ptr<Requirement> ECON_248 = memory::make_unique<Requirement>(
				"ECON_248", "The base price for Rubber shall be 7",
				"The base price of 7 for Rubber can be seen in program output data"
			);
			add_requirement(std::move(ECON_248));
			memory::unique_ptr<Requirement> ECON_249 = memory::make_unique<Requirement>(
				"ECON_249", "The base price for Silk shall be 10",
				"The base price of 10 for Silk can be seen in program output data"
			);
			add_requirement(std::move(ECON_249));
			memory::unique_ptr<Requirement> ECON_250 = memory::make_unique<Requirement>(
				"ECON_250", "The base price for Small Arms shall be 37",
				"The base price of 37 for Small Arms can be seen in program output data"
			);
			add_requirement(std::move(ECON_250));
			memory::unique_ptr<Requirement> ECON_251 = memory::make_unique<Requirement>(
				"ECON_251", "The base price for Steamer Convoys shall be 65",
				"The base price of 65 for Steamer Convoys can be seen in program output data"
			);
			add_requirement(std::move(ECON_251));
			memory::unique_ptr<Requirement> ECON_252 = memory::make_unique<Requirement>(
				"ECON_252", "The base price for Steel shall be 4.7",
				"The base price of 4.7 for Steel can be seen in program output data"
			);
			add_requirement(std::move(ECON_252));
			memory::unique_ptr<Requirement> ECON_253 = memory::make_unique<Requirement>(
				"ECON_253", "The base price for Sulphur shall be 6",
				"The base price of 6 for Sulphur can be seen in program output data"
			);
			add_requirement(std::move(ECON_253));
			memory::unique_ptr<Requirement> ECON_254 = memory::make_unique<Requirement>(
				"ECON_254", "The base price for Tanks shall be 98",
				"The base price of 98 for Tanks can be seen in program output data"
			);
			add_requirement(std::move(ECON_254));
			memory::unique_ptr<Requirement> ECON_255 = memory::make_unique<Requirement>(
				"ECON_255", "The base price for Tea shall be 2.6",
				"The base price of 2.6 for Tea can be seen in program output data"
			);
			add_requirement(std::move(ECON_255));
			memory::unique_ptr<Requirement> ECON_256 = memory::make_unique<Requirement>(
				"ECON_256", "The base price for Telephones shall be 16",
				"The base price of 16 for Telephones can be seen in program output data"
			);
			add_requirement(std::move(ECON_256));
			memory::unique_ptr<Requirement> ECON_257 = memory::make_unique<Requirement>(
				"ECON_257", "The base price for Timber shall be 0.9",
				"The base price of 0.9 for Timber can be seen in program output data"
			);
			add_requirement(std::move(ECON_257));
			memory::unique_ptr<Requirement> ECON_258 = memory::make_unique<Requirement>(
				"ECON_258", "The base price for Tobacco shall be 1.1",
				"The base price of 1.1 for Tobacco can be seen in program output data"
			);
			add_requirement(std::move(ECON_258));
			memory::unique_ptr<Requirement> ECON_259 = memory::make_unique<Requirement>(
				"ECON_259", "The base price for Tropical Wood shall be 5.4",
				"The base price of 5.4 for Tropical Wood can be seen in program output data"
			);
			add_requirement(std::move(ECON_259));
			memory::unique_ptr<Requirement> ECON_260 = memory::make_unique<Requirement>(
				"ECON_260", "The base price for Wine shall be 9.7",
				"The base price of 9.7 for Wine can be seen in program output data"
			);
			add_requirement(std::move(ECON_260));
			memory::unique_ptr<Requirement> ECON_261 = memory::make_unique<Requirement>(
				"ECON_261", "The base price for Wool shall be 0.7",
				"The base price of 0.7 for Wool can be seen in program output data"
			);
			add_requirement(std::move(ECON_261));
		}

		void execute_script() {
			// Reference of goods by identifier:
			// ammunition small_arms artillery canned_food barrels aeroplanes cotton dye wool
			// silk coal sulphur iron timber tropical_wood rubber oil precious_metal steel cement
			// machine_parts glass fuel fertilizer explosives clipper_convoy steamer_convoy
			// electric_gear fabric lumber paper cattle fish fruit grain tobacco tea coffee opium
			// automobiles telephones wine liquor regular_clothes luxury_clothes furniture
			// luxury_furniture radio

			// ECON_123
			// The base price for Aeroplanes shall be 110
			// The base price of 110 for Aeroplanes can be seen in program output data

			check_base_price("aeroplanes", "110.0", "ECON_123");

			// ECON_124
			// The base price for Ammunition shall be 17.5
			// The base price of 17.5 for Ammunition can be seen in program output data

			check_base_price("ammunition", "17.5", "ECON_124");

			// ECON_125
			// The base price for Artillery shall be 60
			// The base price of 60 for Artillery can be seen in program output data

			check_base_price("artillery", "60.0", "ECON_125");

			// ECON_126
			// The base price for Automobiles shall be 70
			// The base price of 70 for Automobiles can be seen in program output data

			check_base_price("automobiles", "70.0", "ECON_126");

			// ECON_127
			// The base price for Canned Food shall be 16
			// The base price of 16 for Canned Food can be seen in program output data

			check_base_price("canned_food", "16.0", "ECON_127");

			// ECON_128
			// The base price for Cattle shall be 2
			// The base price of 2 for Cattle can be seen in program output data

			check_base_price("cattle", "2.0", "ECON_128");

			// ECON_129
			// The base price for Cement shall be 16
			// The base price of 16 for Cement can be seen in program output data

			check_base_price("cement", "16.0", "ECON_129");

			// ECON_130
			// The base price for Clipper Convoys shall be 42
			// The base price of 42 for Clipper Convoys can be seen in program output data

			check_base_price("clipper_convoy", "42.0", "ECON_130");

			// ECON_131
			// The base price for Coal shall be 2.3
			// The base price of 2.3 for Coal can be seen in program output data

			check_base_price("coal", "2.3", "ECON_131");

			// ECON_132
			// The base price for Coffee shall be 2.1
			// The base price of 2.1 for Coffee can be seen in program output data

			check_base_price("coffee", "2.1", "ECON_132");

			// ECON_133
			// The base price for Cotton shall be 2
			// The base price of 2 for Cotton can be seen in program output data

			check_base_price("cotton", "2.0", "ECON_133");

			// ECON_134
			// The base price for Dye shall be 12
			// The base price of 12 for Dye can be seen in program output data

			check_base_price("dye", "12.0", "ECON_134");

			// ECON_135
			// The base price for Electric Gear shall be 16
			// The base price of 16 for Electric Gear can be seen in program output data

			check_base_price("electric_gear", "16.0", "ECON_135");

			// ECON_136
			// The base price for Explosives shall be 20
			// The base price of 20 for Explosives can be seen in program output data

			check_base_price("explosives", "20.0", "ECON_136");

			// ECON_137
			// The base price for Fabric shall be 1.8
			// The base price of 1.8 for Fabric can be seen in program output data

			check_base_price("fabric", "1.8", "ECON_137");

			// ECON_138
			// The base price for Fertilizer shall be 10
			// The base price of 10 for Fertilizer can be seen in program output data

			check_base_price("fertilizer", "10.0", "ECON_138");

			// ECON_139
			// The base price for Fish shall be 1.5
			// The base price of 1.5 for Fish can be seen in program output data

			check_base_price("fish", "1.5", "ECON_139");

			// ECON_140
			// The base price for Fruit shall be 1.8
			// The base price of 1.8 for Fruit can be seen in program output data

			check_base_price("fruit", "1.8", "ECON_140");

			// ECON_141
			// The base price for Fuel shall be 25
			// The base price of 25 for Fuel can be seen in program output data

			check_base_price("fuel", "25.0", "ECON_141");

			// ECON_142
			// The base price for Furniture shall be 4.9
			// The base price of 4.9 for Furniture can be seen in program output data

			check_base_price("furniture", "4.9", "ECON_142");

			// ECON_234
			// The base price for Glass shall be 2.9
			// The base price of 2.9 for Glass can be seen in program output data

			check_base_price("glass", "2.9", "ECON_234");

			// ECON_235
			// The base price for Grain shall be 2.2
			// The base price of 2.2 for Grain can be seen in program output data

			check_base_price("grain", "2.2", "ECON_235");

			// ECON_236
			// The base price for Iron shall be 3.5
			// The base price of 3.5 for Iron can be seen in program output data

			check_base_price("iron", "3.5", "ECON_236");

			// ECON_237
			// The base price for Liquor shall be 6.4
			// The base price of 6.4 for Liquor can be seen in program output data

			check_base_price("liquor", "6.4", "ECON_237");

			// ECON_238
			// The base price for Lumber shall be 1
			// The base price of 1 for Lumber can be seen in program output data

			check_base_price("lumber", "1.0", "ECON_238");

			// ECON_239
			// The base price for Luxury Clothes shall be 65
			// The base price of 65 for Luxury Clothes can be seen in program output data

			check_base_price("luxury_clothes", "65.0", "ECON_239");

			// ECON_240
			// The base price for Luxury Furniture shall be 59
			// The base price of 59 for Luxury Furniture can be seen in program output data

			check_base_price("luxury_furniture", "59.0", "ECON_240");

			// ECON_241
			// The base price for Machine Parts shall be 36.5
			// The base price of 36.5 for Machine Parts can be seen in program output data

			check_base_price("machine_parts", "36.5", "ECON_241");

			// ECON_242
			// The base price for Oil shall be 12
			// The base price of 12 for Oil can be seen in program output data

			check_base_price("oil", "12.0", "ECON_242");

			// ECON_243
			// The base price for Opium shall be 3.2
			// The base price of 3.2 for Opium can be seen in program output data

			check_base_price("opium", "3.2", "ECON_243");

			// ECON_244
			// The base price for Paper shall be 3.4
			// The base price of 3.4 for Paper can be seen in program output data

			check_base_price("paper", "3.4", "ECON_244");

			// ECON_245
			// The base price for Precious Metal shall be 8
			// The base price of 8 for Precious Metal can be seen in program output data

			check_base_price("precious_metal", "8.0", "ECON_245");

			// ECON_246
			// The base price for Radios shall be 16
			// The base price of 16 for Radios can be seen in program output data

			check_base_price("radio", "16.0", "ECON_246");

			// ECON_247
			// The base price for Regular Clothes shall be 5.8
			// The base price of 5.8 for Regular Clothes can be seen in program output data

			check_base_price("regular_clothes", "5.8", "ECON_247");

			// ECON_248
			// The base price for Rubber shall be 7
			// The base price of 7 for Rubber can be seen in program output data

			check_base_price("rubber", "7.0", "ECON_248");

			// ECON_249
			// The base price for Silk shall be 10
			// The base price of 10 for Silk can be seen in program output data

			check_base_price("silk", "10.0", "ECON_249");

			// ECON_250
			// The base price for Small Arms shall be 37
			// The base price of 37 for Small Arms can be seen in program output data

			check_base_price("small_arms", "37.0", "ECON_250");

			// ECON_251
			// The base price for Steamer Convoys shall be 65
			// The base price of 65 for Steamer Convoys can be seen in program output data

			check_base_price("steamer_convoy", "65.0", "ECON_251");

			// ECON_252
			// The base price for Steel shall be 4.7
			// The base price of 4.7 for Steel can be seen in program output data

			check_base_price("steel", "4.7", "ECON_252");

			// ECON_253
			// The base price for Sulphur shall be 6
			// The base price of 6 for Sulphur can be seen in program output data

			check_base_price("sulphur", "6.0", "ECON_253");

			// ECON_254
			// The base price for Tanks shall be 98
			// The base price of 98 for Tanks can be seen in program output data

			check_base_price("barrels", "98.0", "ECON_254");

			// ECON_255
			// The base price for Tea shall be 2.6
			// The base price of 2.6 for Tea can be seen in program output data

			check_base_price("tea", "2.6", "ECON_255");

			// ECON_256
			// The base price for Telephones shall be 16
			// The base price of 16 for Telephones can be seen in program output data

			check_base_price("telephones", "16.0", "ECON_256");

			// ECON_257
			// The base price for Timber shall be 0.9
			// The base price of 0.9 for Timber can be seen in program output data

			check_base_price("timber", "0.9", "ECON_257");

			// ECON_258
			// The base price for Tobacco shall be 1.1
			// The base price of 1.1 for Tobacco can be seen in program output data

			check_base_price("tobacco", "1.1", "ECON_258");

			// ECON_259
			// The base price for Tropical Wood shall be 5.4
			// The base price of 5.4 for Tropical Wood can be seen in program output data

			check_base_price("tropical_wood", "5.4", "ECON_259");

			// ECON_260
			// The base price for Wine shall be 9.7
			// The base price of 9.7 for Wine can be seen in program output data

			check_base_price("wine", "9.7", "ECON_260");

			// ECON_261
			// The base price for Wool shall be 0.7
			// The base price of 0.7 for Wool can be seen in program output data

			check_base_price("wool", "0.7", "ECON_261");
		}

		void check_base_price(memory::string identifier, memory::string target_value, memory::string req_name) {
			// Get string of base_price from goods manager
			fixed_point_t base_price_fp = get_definition_manager()
											  ->get_economy_manager()
											  .get_good_definition_manager()
											  .get_good_definition_by_identifier(identifier)
											  ->base_price;
			memory::string base_price = memory::fmt::format("{:.1f}", base_price_fp);

			// Perform req checks
			pass_or_fail_req_with_actual_and_target_values(req_name, target_value, base_price);
		}
	};
}
