#pragma once

/** @file atomic_output.hpp @brief Commit a complete output file with rename. */
#include <filesystem>
#include <functional>
#include <iosfwd>

namespace course {

/**
 * @brief Stage output, then replace the destination after successful close.
 * @param destination New path or an existing ordinary file.
 * @param writer Callback that writes the complete contents to the given stream.
 * @throws std::runtime_error On temporary-file, stream, or replacement errors.
 * @details Symlinks, including dangling links, and nonregular existing targets
 * are rejected. An exclusively created directory in the destination's parent
 * holds the temporary file, so replacement stays on one filesystem. On POSIX,
 * the directory is owner-only and existing ordinary-file mode bits are copied.
 * On Windows, the directory inherits its parent's ACL and MoveFileExW replaces
 * the result without first deleting it; ACL preservation is not promised.
 * Callback exceptions propagate; temporary resources are cleaned by RAII.
 * Visibility depends on the filesystem's rename semantics. Power-loss
 * durability and preservation of all metadata are not guaranteed. External
 * concurrent changes to the destination or its parent are not coordinated.
 */
void write_file_atomically(const std::filesystem::path& destination,
                           const std::function<void(std::ostream&)>& writer);

}  // namespace course
