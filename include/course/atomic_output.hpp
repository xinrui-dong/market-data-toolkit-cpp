#pragma once

/** @file atomic_output.hpp @brief Commit a complete output file with rename. */
#include <filesystem>
#include <functional>
#include <iosfwd>

namespace course {

/**
 * @brief Write privately, then replace the destination after successful close.
 * @param destination New path or an existing ordinary file.
 * @param writer Callback that writes the complete contents to the given stream.
 * @throws std::runtime_error On temporary-file, stream, or replacement errors.
 * @details Symlinks, including dangling links, and nonregular existing targets
 * are rejected. An exclusively created private directory in the destination's
 * parent holds the temporary file, so the final rename stays on one filesystem.
 * Existing ordinary-file permission bits are preserved. Callback exceptions
 * propagate; temporary resources are cleaned by RAII.
 * This provides atomic visibility on the supported Mac/Linux filesystems,
 * not power-loss durability or preservation of all file metadata. External
 * concurrent changes to the destination or its parent are not coordinated.
 */
void write_file_atomically(const std::filesystem::path& destination,
                           const std::function<void(std::ostream&)>& writer);

}  // namespace course
