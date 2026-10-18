"""Independent HTML fixtures for the generated-documentation link checker."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))
from check_docs import validate_docs  # noqa: E402


class DocsValidationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "docs"
        self.root.mkdir()

    def write(self, filename, content):
        path = self.root / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def test_anchors_encoded_urls_queries_and_directory_index(self):
        self.write("index.html", '''
            <div id="same&amp;page"></div><a href="#same%26page">same</a>
            <a href="API%20%D0%BE%D0%BF%D0%B8%D1%81%D0%B0%D0%BD%D0%B8%D0%B5.htm?q=1&amp;x=2#price%20return">API</a>
            <a href="guide/?query=value#legacy&amp;name">guide</a>
            <link href="style.css?version=2" rel="stylesheet">
            <img src="logo.svg#drawing"/><script src="app.js"></script>
        ''')
        self.write("API описание.htm", '<section id="price return"></section>')
        self.write("guide/index.html", '<a name="legacy&amp;name"></a>')
        for filename in ("style.css", "logo.svg", "app.js"):
            self.write(filename, "fixture")
        result = validate_docs(self.root)
        self.assertEqual(result.errors, [])
        self.assertEqual(result.html_pages, 3)
        self.assertEqual(result.local_references, 6)

    def test_missing_file_and_asset_are_reported(self):
        self.write("index.html", '<a href="absent.html">bad</a><img src="absent.png">')
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 2)
        self.assertTrue(all("missing local file" in error for error in result.errors))

    def test_missing_same_page_and_other_page_anchors(self):
        self.write("index.html", '<a href="#missing">bad</a><a href="api.html#missing">bad</a>')
        self.write("api.html", '<p id="present"></p>')
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 2)
        self.assertTrue(all("missing anchor #missing" in error for error in result.errors))

    def test_doxygen_118_member_letter_navigation_regression(self):
        self.write("index.html", '<a href="functions.html">Members</a>')
        for name in ("functions.html", "functions_vars.html"):
            self.write(name, '''
                <a href="#index_c">c</a><a href="#index_d">d</a>
                <h3><a id="letter_c"></a>c</h3><h3><a id="letter_d"></a>d</h3>
            ''')
        before = {p: p.read_bytes() for p in self.root.glob("*.html")}
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 4)
        for name in ("functions.html", "functions_vars.html"):
            for anchor in ("index_c", "index_d"):
                self.assertTrue(any(name in error and f"missing anchor #{anchor}" in error
                                    for error in result.errors))
        self.assertEqual(before, {p: p.read_bytes() for p in self.root.glob("*.html")})

    def test_all_schemes_and_network_locations_are_skipped(self):
        self.write("index.html", '''
            <a href="https://example.invalid/absent#missing">https</a>
            <a href="http://example.invalid/absent">http</a>
            <a href="//example.invalid/absent">network</a>
            <a href="mailto:example@example.invalid">mail</a>
            <a href="javascript:void(0)">script</a><img src="data:image/png;base64,AAAA">
        ''')
        result = validate_docs(self.root)
        self.assertEqual(result.errors, [])
        self.assertEqual(result.local_references, 0)

    def test_relative_escape_is_rejected_even_when_file_exists(self):
        (self.root.parent / "outside.html").write_text("present", encoding="utf-8")
        self.write("index.html", '<a href="%2e%2e/outside.html">outside</a>')
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 1)
        self.assertIn("escapes documentation root", result.errors[0])

    def test_symlink_asset_escape_is_rejected(self):
        outside = self.root.parent / "outside.png"
        outside.write_bytes(b"fixture")
        try:
            (self.root / "image.png").symlink_to(outside)
        except (NotImplementedError, OSError) as error:
            self.skipTest(f"symlinks unavailable: {error}")
        self.write("index.html", '<img src="image.png">')
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 1)
        self.assertIn("escapes documentation root", result.errors[0])

    def test_directory_without_index_is_missing_file(self):
        (self.root / "guide").mkdir()
        self.write("index.html", '<a href="guide/">guide</a>')
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 1)
        self.assertIn("missing local file guide/index.html", result.errors[0])

    def test_empty_directory_fails(self):
        result = validate_docs(self.root)
        self.assertTrue(any("entry point" in error for error in result.errors))
        self.assertTrue(any("no HTML pages" in error for error in result.errors))

    def test_html_without_entry_point_fails(self):
        self.write("api.html", '<p id="present"></p>')
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 1)
        self.assertIn("entry point: index.html", result.errors[0])

    def test_absent_directory_fails(self):
        result = validate_docs(self.root / "absent")
        self.assertEqual(len(result.errors), 1)
        self.assertIn("directory does not exist", result.errors[0])

    def test_unreadable_html_encoding_fails(self):
        (self.root / "index.html").write_bytes(b"\xff\xfe")
        result = validate_docs(self.root)
        self.assertEqual(len(result.errors), 1)
        self.assertIn("cannot read HTML", result.errors[0])

    def test_cli_failure_and_success_exit_status(self):
        self.write("index.html", '<a href="#missing">bad</a>')
        command = [sys.executable, str(SCRIPTS / "check_docs.py"), str(self.root)]
        failed = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertEqual(failed.returncode, 1)
        self.assertEqual(failed.stdout, "")
        self.assertIn("missing anchor #missing", failed.stderr)
        self.write("index.html", '<a href="#present">good</a><p id="present"></p>')
        passed = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertEqual(passed.returncode, 0)
        self.assertEqual(passed.stderr, "")
        self.assertIn("1 HTML pages, 1 local references", passed.stdout)


if __name__ == "__main__":
    unittest.main()
