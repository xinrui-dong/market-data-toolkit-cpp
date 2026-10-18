#include "course/atomic_output.hpp"

#include <fstream>
#include <iomanip>
#include <random>
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

namespace course {
namespace {

namespace fs = std::filesystem;

[[noreturn]] void file_error(const std::string& message,
                             const std::error_code& error) {
    throw std::runtime_error(message + ": " + error.message());
}

fs::file_status check_destination(const fs::path& destination) {
    std::error_code error;
    const auto status = fs::symlink_status(destination, error);
    if (status.type() == fs::file_type::not_found) {
        return status;
    }
    if (error) {
        file_error("could not inspect output file", error);
    }
    if (fs::is_symlink(status)) {
        throw std::runtime_error("output must not be a symbolic link");
    }
    if (!fs::is_regular_file(status)) {
        throw std::runtime_error("output must be a regular file");
    }
    return status;
}

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(const fs::path& parent) {
        std::random_device random;
        for (int attempt = 0; attempt < 64; ++attempt) {
            std::ostringstream name;
            name << ".market-data-output-" << std::hex << std::setfill('0');
            for (int part = 0; part < 4; ++part) {
                name << std::setw(8) << random();
            }
            auto candidate = parent / name.str();
            std::error_code error;
            // Only a successful create claims this directory. A guessed or
            // colliding name is never opened or reused.
            if (!fs::create_directory(candidate, error)) {
                if (!error || error == std::errc::file_exists) {
                    continue;
                }
                file_error("could not create temporary output directory", error);
            }
            path_ = std::move(candidate);
#ifndef _WIN32
            // Windows directories inherit their parent's ACL; POSIX mode bits
            // cannot express an equivalent Windows access-control policy.
            fs::permissions(path_, fs::perms::owner_all, fs::perm_options::replace, error);
            if (error) {
                cleanup();
                file_error("could not protect temporary output directory", error);
            }
#endif
            const bool empty = fs::is_empty(path_, error);
            if (error || !empty) {
                cleanup();
                throw std::runtime_error("temporary output directory is not empty or accessible");
            }
            return;
        }
        throw std::runtime_error("could not create a unique temporary output directory");
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    ~TemporaryDirectory() { cleanup(); }

    const fs::path& path() const { return path_; }

private:
    void cleanup() noexcept {
        try {
            if (!path_.empty()) {
                std::error_code ignored;
                fs::remove_all(path_, ignored);
            }
        } catch (...) {
            // Cleanup must not replace the original exception.
        }
    }

    fs::path path_;
};

}  // namespace

void write_file_atomically(const std::filesystem::path& destination,
                           const std::function<void(std::ostream&)>& writer) {
    if (destination.empty() || destination.filename().empty()) {
        throw std::runtime_error("output filename is required");
    }
    const auto output_path = fs::absolute(destination);
    check_destination(output_path);
    TemporaryDirectory temporary(output_path.parent_path());
    const auto temporary_file = temporary.path() / "output.tmp";

    std::ofstream output(temporary_file, std::ios::out | std::ios::binary);
    if (!output) {
        throw std::runtime_error("could not create temporary output file");
    }
    writer(output);
    output.flush();
    if (!output) {
        throw std::runtime_error("could not write temporary output file");
    }
    output.close();
    if (!output) {
        throw std::runtime_error("could not close temporary output file");
    }

    // Recheck the target after writing. Never follow a destination symlink.
    const auto status = check_destination(output_path);
#ifdef _WIN32
    (void)status;
    // Require a same-volume Windows replacement without first deleting the old
    // result. Unlike MSVC's filesystem::rename implementation, this deliberately
    // omits MOVEFILE_COPY_ALLOWED and its copy-and-delete fallback.
    if (!MoveFileExW(temporary_file.c_str(), output_path.c_str(),
                     MOVEFILE_REPLACE_EXISTING)) {
        const auto native_error = GetLastError();
        file_error("could not replace output file",
                   std::error_code(static_cast<int>(native_error),
                                   std::system_category()));
    }
#else
    std::error_code error;
    if (fs::is_regular_file(status)) {
        fs::permissions(temporary_file, status.permissions(), fs::perm_options::replace, error);
        if (error) {
            file_error("could not preserve output permissions", error);
        }
    }
    fs::rename(temporary_file, output_path, error);
    if (error) {
        file_error("could not replace output file", error);
    }
#endif
}

}  // namespace course
