#!/usr/bin/env python3
"""Check local files and HTML anchors in generated Doxygen documentation."""

import argparse
from dataclasses import dataclass, field
from html.parser import HTMLParser
from pathlib import Path
import sys
from typing import Dict, List, Set, Tuple
from urllib.parse import unquote, urlsplit


class HtmlPage(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.anchors: Set[str] = set()
        self.references: List[Tuple[int, str, str]] = []

    def handle_starttag(self, tag, attrs) -> None:
        for attribute, value in attrs:
            if value is None:
                continue
            if attribute == "id" or (tag == "a" and attribute == "name"):
                self.anchors.add(value)
            if attribute in ("href", "src"):
                self.references.append((self.getpos()[0], attribute, value))


@dataclass
class ValidationResult:
    html_pages: int = 0
    local_references: int = 0
    errors: List[str] = field(default_factory=list)


def validate_docs(directory: Path) -> ValidationResult:
    """Read every HTML page without changing the generated documentation."""
    result = ValidationResult()
    try:
        root = directory.resolve()
        if not root.is_dir():
            result.errors.append(f"Documentation directory does not exist: {root.as_posix()}")
            return result
        if not (root / "index.html").is_file():
            result.errors.append("Missing documentation entry point: index.html")
        html_files = sorted(
            path for path in root.rglob("*")
            if path.is_file() and path.suffix.lower() in (".html", ".htm")
        )
    except (OSError, RuntimeError, ValueError) as error:
        result.errors.append(f"Cannot inspect documentation directory: {error}")
        return result

    result.html_pages = len(html_files)
    if not html_files:
        result.errors.append("Documentation directory contains no HTML pages")

    pages: Dict[Path, HtmlPage] = {}
    for path in html_files:
        label = path.relative_to(root).as_posix()
        try:
            resolved = path.resolve()
            if not resolved.is_relative_to(root):
                result.errors.append(f"{label}: HTML file escapes documentation root")
                continue
            page = HtmlPage()
            page.feed(path.read_text(encoding="utf-8"))
            page.close()
            pages[resolved] = page
        except (OSError, UnicodeError, RuntimeError, ValueError) as error:
            result.errors.append(f"{label}: cannot read HTML: {error}")

    for source, page in pages.items():
        for line, attribute, reference in page.references:
            location = f"{source.relative_to(root).as_posix()}:{line}: {attribute}={reference!r}"
            try:
                url = urlsplit(reference)
                if url.scheme or url.netloc:
                    continue
                result.local_references += 1
                target = (source.parent / unquote(url.path)).resolve() if url.path else source
                if not target.is_relative_to(root):
                    result.errors.append(f"{location}: local reference escapes documentation root")
                    continue
                if target.is_dir():
                    target = (target / "index.html").resolve()
                    if not target.is_relative_to(root):
                        result.errors.append(f"{location}: directory index escapes documentation root")
                        continue
                if not target.is_file():
                    result.errors.append(
                        f"{location}: missing local file {target.relative_to(root).as_posix()}"
                    )
                    continue
                fragment = unquote(url.fragment)
                if fragment and target.suffix.lower() in (".html", ".htm"):
                    target_page = pages.get(target)
                    if target_page is None:
                        result.errors.append(f"{location}: target HTML could not be read")
                    elif fragment not in target_page.anchors:
                        result.errors.append(
                            f"{location}: missing anchor #{fragment} in {target.relative_to(root).as_posix()}"
                        )
            except (OSError, RuntimeError, ValueError) as error:
                result.errors.append(f"{location}: invalid local reference: {error}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, help="generated Doxygen HTML directory")
    args = parser.parse_args()
    result = validate_docs(args.directory)
    if result.errors:
        for error in result.errors:
            print(f"ERROR: {error}", file=sys.stderr)
        print(
            f"Documentation validation failed: {len(result.errors)} error(s), "
            f"{result.html_pages} HTML pages, {result.local_references} local references.",
            file=sys.stderr,
        )
        return 1
    print(
        f"Documentation validation passed: {result.html_pages} HTML pages, "
        f"{result.local_references} local references."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
