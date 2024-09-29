#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/interface/LoadBase.hpp"

namespace OpenVic {

	/*
	All from the /news folder

	For the @2@ syntax

	Consider AILoves
	AILoves = {
		"default"
		"case = { value = 10 } case = { trigger = { news_printing_count = 1 } value = -999 }"
		"AI_LOVES_OTHER_TITLE1 AI_LOVES_OTHER_TITLE2"
		"AI_LOVES_OTHER_DESC1_LONG AI_LOVES_OTHER_DESC2_LONG"
		"AI_LOVES_OTHER_DESC1_MEDIUM AI_LOVES_OTHER_DESC2_MEDIUM"
		"AI_LOVES_OTHER_DESC1_SHORT AI_LOVES_OTHER_DESC2_SHORT"
	}
	@2@ in generator_selector pulls the case
	@3@ pulls the titles
	@4@ pulls the long description
	and so on...
	%1% seems to do the same buts allows for when we want the paste inside string quotes in addition to outside??


	The corresponding pattern has the name "AILoves", which seems to indicate it'll draw 
	from an identifier with name "AILoves" as seen above


	EventGrammar handles a lot of this (esp. trigger and picture)

	*/

	enum class article_size_t { small, medium, large };
	static const string_map_t<article_size_t> article_size = {
		{ "small", article_size_t::small },
		{ "medium", article_size_t::medium },
		{ "large", article_size_t::large }
	};

	using identifier_int_map = deque_ordered_map<std::string,int32_t>;
	using identifier_str_collection_map = deque_ordered_map<std::string,std::vector<std::vector<std::string>>>;

	struct NewsManager;

	class Typed : public Named<> {
	protected:
		Typed() = default;

	public:
		Typed(Typed&&) = default;
		virtual ~Typed() = default;

		OV_DETAIL_GET_BASE_TYPE(Typed)
		OV_DETAIL_GET_TYPE

		static NodeTools::node_callback_t expect_typed_objects(
			NodeTools::length_callback_t length_callback, NodeTools::callback_t<std::unique_ptr<Typed>&&> callback
		);
	};




	/*
	case can have
		0+ trigger = {}
		0+ value = int
		0+ priority_add = int
		0/1 picture

		more specifically, value, priority_add and picture can be thought of as effects, of which there must be one
		having a trigger is optional though
	*/

	struct Case : HasIdentifier {
		//friend struct NewsManager;

	private:
		ConditionScript PROPERTY(trigger);
		//these always appear to be ints, but for consistency with other defines files...
		fixed_point_t PROPERTY(value); 
		fixed_point_t PROPERTY(priority_add);
		std::string PROPERTY(picture);

		Case(size_t new_index, fixed_point_t new_value, fixed_point_t new_priority_add,
		 std::string_view picture_path, ConditionScript&& new_trigger);

		bool parse_scripts(DefinitionManager const& definition_manager);
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map);
		
		//bool load_case(ast::NodeCPtr root);

	public:
		Case(Case&&) = default;
	};


	/*
	TODO
	==== for these 3 (4?), the name and type properties will match ===
	0+ generator_selector
		1 string type
		1 string name = "default"
		0+ case = { value = int }
			case = {trigger = { news_printing_count = 1 value = -999}}
		can have '@2@' ?? if generator_selector is in a pattern
	*/
	class GeneratorSelector final : public Typed {
		friend std::unique_ptr<GeneratorSelector> std::make_unique<GeneratorSelector>();
	//public:

	private:
		Case PROPERTY(generator_case);

	protected:
		GeneratorSelector();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		GeneratorSelector(GeneratorSelector&&) = default;
		virtual ~GeneratorSelector() = default;

		OV_DETAIL_GET_TYPE

	};


	/*
	0+ news_priority
		1 string type
		1 string name = "default"
		0+ case = { 0/1 trigger = {...} priority_add = int }
	*/
	class NewsPriority final : public Typed {
		friend std::unique_ptr<NewsPriority> std::make_unique<NewsPriority>();
	//public:

	private:
		Case PROPERTY(priority_case);

	protected:
		NewsPriority();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		NewsPriority(NewsPriority&&) = default;
		virtual ~NewsPriority() = default;

		OV_DETAIL_GET_TYPE

	};

	/*
	TODO
	0+ generate_article
		1 string type
		1 string name = "default"
		1 size = "small"|"medium"|"large"
		0+ picture_case = {...}
			1+ picture = "news/pic.dds"
			0/1? trigger = { or = { strings_eq = {...} string_eq = ... }}
		1+ title_case (guessing multiple of these means just pick one at random)
			0/1 trigger
			1+ text_add = { 1+ localization_str_identifier? }
		1+ description_case
			0/1 trigger
			1+ text_add = { 1+ localization_str_identifier? }
	*/
	class GenerateArticle final : public Typed {
		friend std::unique_ptr<GenerateArticle> std::make_unique<GenerateArticle>();
	public:
		class PictureCase {
			friend class GenerateArticle;

			std::vector<std::string> PROPERTY(pictures);
			ConditionScript PROPERTY(trigger);

			PictureCase(ConditionScript&& trigger_new, std::vector<std::string> pictures_new);

		public:
			PictureCase(PictureCase&&) = default;
		};

		class TextCase {
			friend class GenerateArticle;

			ConditionScript PROPERTY(trigger);
			std::vector<std::string> PROPERTY(text);

			TextCase(ConditionScript&& trigger_new, std::vector<std::string> text_new);

		public:
			TextCase(TextCase&&) = default;
		};


	private:
		article_size_t PROPERTY(size);
		PictureCase PROPERTY(picture_case);
		TextCase PROPERTY(title_case);
		TextCase PROPERTY(description_case);

	protected:
		GenerateArticle();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		GenerateArticle(GenerateArticle&&) = default;
		virtual ~GenerateArticle() = default;

		OV_DETAIL_GET_TYPE

	};

	/*
	TODO
	0+ pattern
		1 string name **** not the same as the generator/priority/article name
		1 generator_selector
		1 news_priority
		1+ generate_article

	A Pattern object is not necessarily obtained from finding one in the defines,
	the generator selector, news_priority, and article generators can be on their own in the
	file, but are of course linked by sharing the type and name

	*/
	class Pattern final : public Named<> {
		friend std::unique_ptr<Pattern> std::make_unique<Pattern>();

	private:
		GeneratorSelector PROPERTY(selector);
		NewsPriority PROPERTY(priority);
		std::vector<GenerateArticle> PROPERTY(article_formats);

	protected:
		Pattern();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Pattern(Pattern&&) = default;
		virtual ~Pattern() = default;

		OV_DETAIL_GET_TYPE

	};



	/*
	TODO
	===== scope things ====
	0/1? on_printing
		1 string type
		1 string name = "default"
		1 effect
			0+ clear_news_scopes = {type = string limit = {...}}
			0/1+? set_news_flag = string >>> trigger can have has_news_flag = string
	*/
	class OnPrinting final : public Typed {
		friend std::unique_ptr<OnPrinting> std::make_unique<OnPrinting>();
	//public:

	private:
		EffectScript PROPERTY(effect);

	protected:
		OnPrinting();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		OnPrinting(OnPrinting&&) = default;
		virtual ~OnPrinting() = default;

		OV_DETAIL_GET_TYPE

	};



	/*
	TODO
	the difference of on_collection might be that the player is involved?
	0/1 on_collection. desc says "what happens when scope is collected"
		1 string type
		1 effect
			0?+ clear_news_scopes = {type = string limit = { ... }}
	*/
	class OnCollection {
		friend std::unique_ptr<OnCollection> std::make_unique<OnCollection>();
	//public:

	private:
		std::string PROPERTY(type);
		EffectScript PROPERTY(effect);

	protected:
		OnCollection();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map);

	public:
		OnCollection(OnCollection&&) = default;
		virtual ~OnCollection() = default;

		//OV_DETAIL_GET_TYPE

	};


	/*
	TODO
	1 style = {...}
		1 name = "default_style"
		1 trigger = {}
		1 gui_windows = "news_window_default"
		1+ article = { size = large|medium|small	gui_window = "article_SIZE_INT"}
		1 article_limits = { IDENTIFIER = INT IDENTIFIER= INT ... }
		1 title_image = { ... }
			0?+ case
				0/1 trigger
				1 picture = "news/bla.dds"
	*/

	class Style final : public Named<> {
		friend std::unique_ptr<Style> std::make_unique<Style>();
	public:
		class Article {
			friend class Style;
			std::string PROPERTY(gui_window);
			article_size_t PROPERTY(size);

			Article(std::string_view new_gui_window, article_size_t new_size = article_size_t::small);

		public:
			Article(Article&&) = default;
		};

		class TitleImage {
			friend class Style;
			std::vector<Case> PROPERTY(cases);

			TitleImage(std::vector<Case> new_cases);

		public:
			TitleImage(TitleImage&&) = default;
		};

	private:
		ConditionScript PROPERTY(trigger);
		std::string PROPERTY(gui_window);
		std::vector<Article> PROPERTY(articles);
		TitleImage PROPERTY(title_image);
		//using sfx_asset_map_t = deque_ordered_map<godot::StringName, godot::Ref<godot::AudioStreamWAV>>;
		identifier_int_map PROPERTY(article_limits_map);
		// map < IDENTIFIER, INT >

	protected:
		Style();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Style(Style&&) = default;
		virtual ~Style() = default;

		OV_DETAIL_GET_TYPE

	};



	struct NewsManager {
	private:
		//IdentifierRegistry<Pattern> IDENTIFIER_REGISTRY(patterns);

		/*
		TODO
		New GP has "default", "player_is_gp", "player_loss_gp" as identifier, fake names has "int", AI ones have "default" or "player"
		0+ identifier_for_pattern (ie. this could be any random name)
		ex. dino_news_pattern1 or AiAfraidOf
			these correspond to a pattern identifier (the ..._news_pattern will correspond to the name of pattern)
			1 unlabelled string identifier
			0/1 unlabelled string "case string"
			1 unlabelled string "TITLE_IDENTIFIER1 TITLE_IDENTIFIER2 ...."
			0/1? unlabelled string "LONG_DESC1 LONG_DESC2 ..."
			0/1? unlabelled string "MEDIUM_DESC ..."
			1? unlabelled string "SHORT_DESC ...."
		*/
		//>>>>>>> map < identifier, std::vector< std::vector<std::string> > >
		identifier_str_collection_map news_str_collection;//PROPERTY(limits_map);


		/*
		TODO
		news_layout.txt
		1 article_tensions = { IDENTIFIER = int IDENTIFIER = INT ... }, comments say default int is 1
		We should find the identifiers in the various components of a pattern (type property)
		*/
		// map < IDENTIFIER, INT >
		identifier_int_map article_tensions;//PROPERTY(limits_map);

		//Songs.txt
	public:
		bool load_songs_file(ast::NodeCPtr root);
		bool parse_scripts(DefinitionManager const& definition_manager);
	};







/*
	template<typename... Context>
	class Named : public LoadBase<Context...> {
		std::string PROPERTY(name);

	protected:
		Named() = default;

		virtual bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, Context...) override {
			using namespace OpenVic::NodeTools;
			return add_key_map_entries(key_map, "name", ONE_EXACTLY, expect_string(assign_variable_callback_string(name)));
		}

		void _set_name(std::string_view new_name) {
			if (!name.empty()) {
				Logger::warning("Overriding scene name ", name, " with ", new_name);
			}
			name = new_name;
		}

	public:
		Named(Named&&) = default;
		virtual ~Named() = default;

		OV_DETAIL_GET_TYPE
	};
*/


	/*struct NewsManager {
	private:
		IdentifierRegistry<Case> IDENTIFIER_REGISTRY(case);
		//Songs.txt
	public:
		bool load_songs_file(ast::NodeCPtr root);
		bool parse_scripts(DefinitionManager const& definition_manager);
	};*/


/*
	class Actor final : public Object {
		friend std::unique_ptr<Actor> std::make_unique<Actor>();

	public:
		struct attachment....
	private:
		fixed_point_t PROPERTY(scale);
		std::string PROPERTY(model_file);
		std::optional<Animation> PROPERTY(idle_animation);
		std::vector<Attachment> PROPERTY(attachments);

		bool _set_animation(std::string_view name, std::string_view file, fixed_point_t scroll_time);

	protected:
		Actor();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Actor(Actor&&) = default;
		virtual ~Actor() = default;

		OV_DETAIL_GET_TYPE
	};
*/


}

