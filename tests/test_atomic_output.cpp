#include <doctest/doctest.h>

#include "course/atomic_output.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

namespace fs = std::filesystem;

class AtomicTestDirectory {
public:
    AtomicTestDirectory() {
        std::random_device random;
        for (int attempt = 0; attempt < 64; ++attempt) {
            std::ostringstream name;
            name << "course-atomic-test-" << std::hex << random() << random();
            auto candidate = fs::temp_directory_path() / name.str();
            std::error_code error;
            if (fs::create_directory(candidate, error)) {
                path_ = std::move(candidate);
                return;
            }
            if (error && error != std::errc::file_exists) {
                throw std::runtime_error("could not create atomic-output test directory");
            }
        }
        throw std::runtime_error("could not find a unique atomic-output test directory");
    }

    ~AtomicTestDirectory() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }

    const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

void write_text(const fs::path& path, const std::string& text) {
    std::ofstream output(path);
    output << text;
    output.close();
    if (!output) {
        throw std::runtime_error("could not write test fixture");
    }
}

std::string read_text(const fs::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("could not read test fixture");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::set<std::string> entries(const fs::path& directory) {
    std::set<std::string> names;
    for (const auto& entry : fs::directory_iterator(directory)) {
        names.insert(entry.path().filename().string());
    }
    return names;
}

}  // namespace

TEST_CASE("atomic output creates and replaces a complete file and preserves POSIX mode bits") {
    AtomicTestDirectory directory;
    const auto destination = directory.path() / "result.csv";
    course::write_file_atomically(destination, [](std::ostream& output) {
        output << "first complete result\n";
    });
    CHECK(read_text(destination) == "first complete result\n");
    CHECK(entries(directory.path()) == std::set<std::string>{"result.csv"});

#ifndef _WIN32
    fs::permissions(destination, fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace);
    const auto original_permissions = fs::status(destination).permissions();
#else
    std::cout << "POSIX mode-bit assertion unavailable on Windows; "
                 "creation, replacement and cleanup remain checked.\n";
#endif
    course::write_file_atomically(destination, [](std::ostream& output) {
        output << "second complete result\n";
    });
    CHECK(read_text(destination) == "second complete result\n");
#ifndef _WIN32
    CHECK(fs::status(destination).permissions() == original_permissions);
#endif
    CHECK(entries(directory.path()) == std::set<std::string>{"result.csv"});
}

TEST_CASE("atomic output preserves an existing file after a partial write throws") {
    AtomicTestDirectory directory;
    const auto destination = directory.path() / "result.csv";
    write_text(destination, "previous verified result\n");
    CHECK_THROWS_AS(course::write_file_atomically(destination, [](std::ostream& output) {
        output << "partial replacement\n";
        output.flush();
        throw std::runtime_error("simulated writer failure");
    }), std::runtime_error);
    CHECK(read_text(destination) == "previous verified result\n");
    CHECK(entries(directory.path()) == std::set<std::string>{"result.csv"});
}

TEST_CASE("atomic output leaves no new destination after a partial write throws") {
    AtomicTestDirectory directory;
    const auto destination = directory.path() / "result.csv";
    CHECK_THROWS_AS(course::write_file_atomically(destination, [](std::ostream& output) {
        output << "partial new result\n";
        output.flush();
        throw std::runtime_error("simulated writer failure");
    }), std::runtime_error);
    CHECK_FALSE(fs::exists(destination));
    CHECK(entries(directory.path()).empty());
}

TEST_CASE("atomic output detects failed stream state and removes temporary resources") {
    AtomicTestDirectory directory;
    const auto destination = directory.path() / "result.csv";
    write_text(destination, "previous verified result\n");
    CHECK_THROWS_AS(course::write_file_atomically(destination, [](std::ostream& output) {
        output << "partial result\n";
        output.setstate(std::ios::badbit);
    }), std::runtime_error);
    CHECK(read_text(destination) == "previous verified result\n");
    CHECK(entries(directory.path()) == std::set<std::string>{"result.csv"});
}

TEST_CASE("atomic output preserves an existing destination after a real replacement failure") {
    AtomicTestDirectory directory;
    const auto destination = directory.path() / "result.csv";
    write_text(destination, "previous verified result\n");
#ifdef _WIN32
    // A handle without FILE_SHARE_DELETE allows reading the old result but
    // prevents replacement. This exercises the native API's failure path.
    const HANDLE locked = CreateFileW(destination.c_str(), GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                      nullptr);
    REQUIRE(locked != INVALID_HANDLE_VALUE);
#endif
    std::string message;
    try {
        course::write_file_atomically(destination, [&](std::ostream& output) {
            output << "complete temporary result\n";
            output.flush();
#ifndef _WIN32
            // POSIX allows moving an open file. Replace its old path with an
            // empty directory: permission copying succeeds, but replacing the
            // ordinary destination with this directory must fail at rename.
            fs::path temporary_file;
            for (const auto& entry : fs::directory_iterator(directory.path())) {
                if (entry.is_directory()) {
                    for (const auto& child : fs::directory_iterator(entry.path())) {
                        if (child.is_regular_file()) {
                            temporary_file = child.path();
                        }
                    }
                }
            }
            if (temporary_file.empty()) {
                throw std::runtime_error("test could not find the temporary file");
            }
            fs::rename(temporary_file, temporary_file.parent_path() / "moved-data");
            fs::create_directory(temporary_file);
#endif
        });
    } catch (const std::runtime_error& error) {
        message = error.what();
    }
#ifdef _WIN32
    CHECK(CloseHandle(locked) != 0);
#endif
    CHECK(message.find("could not replace output file") != std::string::npos);
    CHECK(read_text(destination) == "previous verified result\n");
    CHECK(entries(directory.path()) == std::set<std::string>{"result.csv"});
}

TEST_CASE("atomic output rejects both live and dangling destination symlinks") {
    AtomicTestDirectory directory;
    const auto referent = directory.path() / "original.csv";
    const auto destination = directory.path() / "linked.csv";
    SUBCASE("live link") { write_text(referent, "original data\n"); }
    SUBCASE("dangling link") { CHECK_FALSE(fs::exists(referent)); }
    std::error_code link_error;
    fs::create_symlink(referent, destination, link_error);
    if (link_error) {
#ifdef _WIN32
        if (link_error.value() == ERROR_PRIVILEGE_NOT_HELD ||
            link_error.value() == ERROR_NOT_SUPPORTED ||
            link_error.value() == ERROR_INVALID_FUNCTION) {
            std::cout << "Destination-symlink check unavailable: "
                      << link_error.message() << '\n';
            return;
        }
#endif
        FAIL("could not create destination symlink: " << link_error.message());
    }
    const auto before = entries(directory.path());
    bool called = false;
    CHECK_THROWS_AS(course::write_file_atomically(destination, [&](std::ostream&) {
        called = true;
    }), std::runtime_error);
    CHECK_FALSE(called);
    CHECK(fs::is_symlink(fs::symlink_status(destination)));
    CHECK(entries(directory.path()) == before);
    if (fs::exists(referent)) {
        CHECK(read_text(referent) == "original data\n");
    }
}

TEST_CASE("atomic output rejects a directory without touching its contents") {
    AtomicTestDirectory directory;
    const auto destination = directory.path() / "result.csv";
    fs::create_directory(destination);
    write_text(destination / "marker", "keep me\n");
    bool called = false;
    CHECK_THROWS_AS(course::write_file_atomically(destination, [&](std::ostream&) {
        called = true;
    }), std::runtime_error);
    CHECK_FALSE(called);
    CHECK(read_text(destination / "marker") == "keep me\n");
    CHECK(entries(directory.path()) == std::set<std::string>{"result.csv"});
}
