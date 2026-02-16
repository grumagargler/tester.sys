#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "files.h"
#include "transform.h"
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <ios>

TEST_CASE ( "strings::toHash" ) {
	CHECK ( strings::toHash ( { "Watcher" } ) == 8626635636475944568 );
}

TEST_CASE ( "files::toHash" ) {
	auto path = std::filesystem::temp_directory_path () / "test.hash";
	std::ofstream file ( path, std::ios::binary );
	file << "Watcher";
	file.close ();
	CHECK ( files::toHash ( path ) == 8626635636475944568 );
}
