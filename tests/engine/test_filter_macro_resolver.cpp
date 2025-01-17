/*
Copyright (C) 2020 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "filter_macro_resolver.h"
#include <catch.hpp>

using namespace std;
using namespace libsinsp::filter::ast;

TEST_CASE("Should resolve macros on a filter AST", "[rule_loader]")
{
	string macro_name = "test_macro";

	SECTION("in the general case")
	{
		std::shared_ptr<expr> macro = std::move(
			unary_check_expr::create("test.field", "", "exists"));

		std::vector<std::unique_ptr<expr>> filter_and;
		filter_and.push_back(unary_check_expr::create("evt.name", "", "exists"));
		filter_and.push_back(not_expr::create(value_expr::create(macro_name)));
		std::shared_ptr<expr> filter = std::move(and_expr::create(filter_and));

		std::vector<std::unique_ptr<expr>> expected_and;
		expected_and.push_back(unary_check_expr::create("evt.name", "", "exists"));
		expected_and.push_back(not_expr::create(clone(macro.get())));
		std::shared_ptr<expr> expected = std::move(and_expr::create(expected_and));

		filter_macro_resolver resolver;
		resolver.set_macro(macro_name, macro);

		// first run
		REQUIRE(resolver.run(filter) == true);
		REQUIRE(resolver.get_resolved_macros().size() == 1);
		REQUIRE(*resolver.get_resolved_macros().begin() == macro_name);
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(expected.get()));

		// second run
		REQUIRE(resolver.run(filter) == false);
		REQUIRE(resolver.get_resolved_macros().empty());
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(expected.get()));
	}

	SECTION("with a single node")
	{
		std::shared_ptr<expr> macro = std::move(
			unary_check_expr::create("test.field", "", "exists"));

		std::shared_ptr<expr> filter = std::move(value_expr::create(macro_name));

		filter_macro_resolver resolver;
		resolver.set_macro(macro_name, macro);

		// first run
		expr* old_filter_ptr = filter.get();
		REQUIRE(resolver.run(filter) == true);
		REQUIRE(filter.get() != old_filter_ptr);
		REQUIRE(resolver.get_resolved_macros().size() == 1);
		REQUIRE(*resolver.get_resolved_macros().begin() == macro_name);
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(macro.get()));

		// second run
		old_filter_ptr = filter.get();
		REQUIRE(resolver.run(filter) == false);
		REQUIRE(filter.get() == old_filter_ptr);
		REQUIRE(resolver.get_resolved_macros().empty());
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(macro.get()));
	}

	SECTION("with multiple macros")
	{
		string a_macro_name = macro_name + "_1";
		string b_macro_name = macro_name + "_2";

		std::shared_ptr<expr> a_macro = std::move(
			unary_check_expr::create("one.field", "", "exists"));
		std::shared_ptr<expr> b_macro = std::move(
			unary_check_expr::create("another.field", "", "exists"));

		std::vector<std::unique_ptr<expr>> filter_or;
		filter_or.push_back(value_expr::create(a_macro_name));
		filter_or.push_back(value_expr::create(b_macro_name));
		std::shared_ptr<expr> filter = std::move(or_expr::create(filter_or));

		std::vector<std::unique_ptr<expr>> expected_or;
		expected_or.push_back(clone(a_macro.get()));
		expected_or.push_back(clone(b_macro.get()));
		std::shared_ptr<expr>  expected_filter = std::move(or_expr::create(expected_or));

		filter_macro_resolver resolver;
		resolver.set_macro(a_macro_name, a_macro);
		resolver.set_macro(b_macro_name, b_macro);

		// first run
		REQUIRE(resolver.run(filter) == true);
		REQUIRE(resolver.get_resolved_macros().size() == 2);
		REQUIRE(resolver.get_resolved_macros().find(a_macro_name)
				!= resolver.get_resolved_macros().end());
		REQUIRE(resolver.get_resolved_macros().find(b_macro_name)
				!= resolver.get_resolved_macros().end());
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(expected_filter.get()));

		// second run
		REQUIRE(resolver.run(filter) == false);
		REQUIRE(resolver.get_resolved_macros().empty());
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(expected_filter.get()));
	}

	SECTION("with nested macros")
	{
		string a_macro_name = macro_name + "_1";
		string b_macro_name = macro_name + "_2";

		std::vector<std::unique_ptr<expr>> a_macro_and;
		a_macro_and.push_back(unary_check_expr::create("one.field", "", "exists"));
		a_macro_and.push_back(value_expr::create(b_macro_name));
		std::shared_ptr<expr> a_macro = std::move(and_expr::create(a_macro_and));

		std::shared_ptr<expr> b_macro = std::move(
			unary_check_expr::create("another.field", "", "exists"));

		std::shared_ptr<expr> filter = std::move(value_expr::create(a_macro_name));

		std::vector<std::unique_ptr<expr>> expected_and;
		expected_and.push_back(unary_check_expr::create("one.field", "", "exists"));
		expected_and.push_back(unary_check_expr::create("another.field", "", "exists"));
		std::shared_ptr<expr> expected_filter = std::move(and_expr::create(expected_and));

		filter_macro_resolver resolver;
		resolver.set_macro(a_macro_name, a_macro);
		resolver.set_macro(b_macro_name, b_macro);

		// first run
		REQUIRE(resolver.run(filter) == true);
		REQUIRE(resolver.get_resolved_macros().size() == 2);
		REQUIRE(resolver.get_resolved_macros().find(a_macro_name)
				!= resolver.get_resolved_macros().end());
		REQUIRE(resolver.get_resolved_macros().find(b_macro_name)
				!= resolver.get_resolved_macros().end());
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(expected_filter.get()));

		// second run
		REQUIRE(resolver.run(filter) == false);
		REQUIRE(resolver.get_resolved_macros().empty());
		REQUIRE(resolver.get_unknown_macros().empty());
		REQUIRE(filter->is_equal(expected_filter.get()));
	}
}

TEST_CASE("Should find unknown macros", "[rule_loader]")
{
	string macro_name = "test_macro";

	SECTION("in the general case")
	{
		std::vector<std::unique_ptr<expr>> filter_and;
		filter_and.push_back(unary_check_expr::create("evt.name", "", "exists"));
		filter_and.push_back(not_expr::create(value_expr::create(macro_name)));
		std::shared_ptr<expr> filter = std::move(and_expr::create(filter_and));

		filter_macro_resolver resolver;
		REQUIRE(resolver.run(filter) == false);
		REQUIRE(resolver.get_unknown_macros().size() == 1);
		REQUIRE(*resolver.get_unknown_macros().begin() == macro_name);
		REQUIRE(resolver.get_resolved_macros().empty());
	}

	SECTION("with nested macros")
	{
		string a_macro_name = macro_name + "_1";
		string b_macro_name = macro_name + "_2";

		std::vector<std::unique_ptr<expr>> a_macro_and;
		a_macro_and.push_back(unary_check_expr::create("one.field", "", "exists"));
		a_macro_and.push_back(value_expr::create(b_macro_name));
		std::shared_ptr<expr> a_macro = std::move(and_expr::create(a_macro_and));

		std::shared_ptr<expr> filter = std::move(value_expr::create(a_macro_name));
		auto expected_filter = clone(a_macro.get());

		filter_macro_resolver resolver;
		resolver.set_macro(a_macro_name, a_macro);

		// first run
		REQUIRE(resolver.run(filter) == true);
		REQUIRE(resolver.get_resolved_macros().size() == 1);
		REQUIRE(*resolver.get_resolved_macros().begin() == a_macro_name);
		REQUIRE(resolver.get_unknown_macros().size() == 1);
		REQUIRE(*resolver.get_unknown_macros().begin() == b_macro_name);
		REQUIRE(filter->is_equal(expected_filter.get()));
	}
}

TEST_CASE("Should undefine macro", "[rule_loader]")
{
	string macro_name = "test_macro";
	std::shared_ptr<expr> macro = std::move(unary_check_expr::create("test.field", "", "exists"));
	std::shared_ptr<expr> a_filter = std::move(value_expr::create(macro_name));
	std::shared_ptr<expr> b_filter = std::move(value_expr::create(macro_name));
	filter_macro_resolver resolver;

	resolver.set_macro(macro_name, macro);
	REQUIRE(resolver.run(a_filter) == true);
	REQUIRE(resolver.get_resolved_macros().size() == 1);
	REQUIRE(*resolver.get_resolved_macros().begin() == macro_name);
	REQUIRE(resolver.get_unknown_macros().empty());
	REQUIRE(a_filter->is_equal(macro.get()));

	resolver.set_macro(macro_name, NULL);
	REQUIRE(resolver.run(b_filter) == false);
	REQUIRE(resolver.get_resolved_macros().empty());
	REQUIRE(resolver.get_unknown_macros().size() == 1);
	REQUIRE(*resolver.get_unknown_macros().begin() == macro_name);
}

// checks that the macro AST is cloned and not shared across resolved filters
TEST_CASE("Should clone macro AST", "[rule_loader]")
{
	string macro_name = "test_macro";
	std::shared_ptr<unary_check_expr> macro = std::move(unary_check_expr::create("test.field", "", "exists"));
	std::shared_ptr<expr> filter = std::move(value_expr::create(macro_name));
	filter_macro_resolver resolver;
	
	resolver.set_macro(macro_name, macro);
	REQUIRE(resolver.run(filter) == true);
	REQUIRE(resolver.get_resolved_macros().size() == 1);
	REQUIRE(*resolver.get_resolved_macros().begin() == macro_name);
	REQUIRE(resolver.get_unknown_macros().empty());
	REQUIRE(filter->is_equal(macro.get()));

	macro->field = "another.field";
	REQUIRE(!filter->is_equal(macro.get()));
}
