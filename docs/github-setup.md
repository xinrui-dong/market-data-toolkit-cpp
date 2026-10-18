# GitHub Actions and documentation deployment

This project validates historical price CSV data, calculates simple price returns and trailing SMA, and compares two SMA algorithms. This guide describes the repository layout, CI checks and documentation deployment. The first successful publication on 2026-10-14 is recorded separately below.

[Public repository](https://github.com/xinrui-dong/market-data-toolkit-cpp) · [Documentation](https://xinrui-dong.github.io/market-data-toolkit-cpp/) · [Actions](https://github.com/xinrui-dong/market-data-toolkit-cpp/actions)

## Repository root and configuration recovery

**The repository root is `market-data-toolkit-cpp/`.** It directly contains `CMakeLists.txt`, `README.md`, `AGENTS.md`, `src/`, `tests/`, `benchmarks/` and `.github/`. A local course workspace may place this repository inside another folder used to organize coursework; that outer folder is not part of the GitHub repository.

A source ZIP downloaded from GitHub may have a version suffix in its directory name. After extraction, run build commands from the directory that directly contains `CMakeLists.txt`. Keep the repository separate from the private course workspace containing lecture notes, assignment PDFs and local environment files. GitHub reads workflows from `.github/workflows/` at the repository root, and the CMake commands below also run from that root.

The public repository already exists. When cloning the project, extracting a release or restoring its publication settings:

1. Clone the public repository linked above or download the source ZIP for a specific release, then enter the repository root.
2. Preserve `.github/`, `.gitignore`, the supplied third-party source archives and their license information.
3. Keep `build/`, generated HTML, local environment directories and credentials out of Git. Retain the supplied dependency archives in their existing locations so a fresh checkout can build.
4. After a push, open **Actions** and inspect every step in the corresponding **Build, test, benchmark and docs** run.
5. When changing functionality, update the tests, benchmarks, README and API documentation as needed, then run the complete verification sequence again.

## What CI runs

The same Ubuntu workflow is triggered by `push`, `pull_request` and manual `workflow_dispatch` events:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --no-tests=error
cmake --build build --target run_benchmarks
cmake --build build --target docs
```

CI installs CMake, G++, Make, Doxygen and Python 3. Documentation validation requires Python 3.9 or newer and uses only its standard library. The unit-test and benchmark dependencies are supplied as source archives, so CI does not install those two libraries through apt. The Actions log records the actual tool versions.

Measured benchmark results appear in the **Run benchmarks** log and in `build/benchmark.json`. Doxygen writes HTML to `build/docs/html/`; the documentation target then checks local `href` and `src` references and HTML anchors, failing on missing targets. A successful run provides `benchmark-results` and `doxygen-html` artifacts with a configured retention period of 14 days. Artifacts are downloadable run outputs; uploading HTML as an artifact does not publish a documentation website.

The relevant version changes are:

- **0.3.0:** expanded SMA windows to 1, 2, 3, 5, 10, 20 and 100, giving 42 workloads with three repetitions each by default. CTest also covers file replacement, exact CLI errors and benchmark-validator regressions. The benchmark target checks all expected raw measurements; missing workloads, duplicate repetition indices or aggregate-only results fail verification.
- **0.3.1:** enabled Doxygen static menus, repaired alphabetical navigation for short member lists and added documentation-validator regressions. The default configuration contains 31 CTest entries. HTML is uploaded only after documentation generation and local-link validation succeed.
- **0.3.2:** corrected directory names and wording in the documentation, retaining the algorithms, 31 CTest entries and the 42-workload, three-repetition benchmark configuration.
- **0.3.3:** uses English documentation filenames and English text throughout the deliverable. The algorithms and CI workflow are unchanged. Check the Actions run for the exact submission commit to establish its validation result.

The initial record below belongs to project version **0.2.0**, which had 22 CTest entries and 18 benchmark workloads. Its evidence is fixed by the listed commit and CI run; it does not establish the result of later commits.

The workflow does not use `continue-on-error` to ignore compilation, test or benchmark failures. `--no-tests=error` prevents an empty test selection from being accepted as a successful test run. A new run cancels an older run for the same ref; runs on other branches continue. For grading criteria 2–5, inspect the completed CI run for the exact commit being submitted, including build, tests, benchmarks and documentation deployment.

## Enable GitHub Pages

The repository variable **`ENABLE_PAGES`** is the explicit publication switch. It has been enabled in this repository, and the initial Pages deployment succeeded. When restoring the configuration, leaving this variable unset or assigning any value other than `true` still runs the build, tests, benchmarks, Doxygen generation and ordinary artifact uploads; Pages artifact upload and deployment are skipped.

Both the Pages publishing source and the repository variable must be configured. An uploaded HTML artifact alone does not publish the website.

To check or restore the Pages configuration:

1. Open **Settings → Pages** in the repository.
2. Under **Build and deployment → Source**, choose **GitHub Actions**. The repository already contains the complete workflow; no additional Pages workflow is needed. [GitHub publishing-source documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)
3. Open **Settings → Secrets and variables → Actions → Variables**, then select **New repository variable**.
4. Enter `ENABLE_PAGES` as the name and lowercase `true` as the value. This is a repository variable, not a secret. [GitHub variables documentation](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/use-variables)
5. Return to **Actions**, open this workflow and choose **Run workflow** for the default branch, or push an actual change to that branch. To rerun the complete process, rerun the workflow rather than only a previously skipped deployment job.
6. Wait for both **CMake, CTest, benchmark and Doxygen** and **Publish Doxygen to GitHub Pages** to succeed.
7. Open the deployment's `page_url` and check the home page and at least one API function page.
8. Put the successfully accessed URL in the README documentation link. The usual format is `https://<username>.github.io/<repository>/`; use the actual deployment URL.
9. Push an actual README or documentation update and confirm that the new default-branch commit triggers deployment automatically.

Pages is deployed only when the switch is enabled and the event is a **push to the default branch or a manual run on the default branch**. Other branches and pull requests still run the complete build checks and save the HTML artifact without replacing the published site.

The deployment job depends on successful build checks and uses the `github-pages` environment with `pages: write` and `id-token: write` permissions. The build job has read-only source access. This follows GitHub's separation of build and deployment jobs. [GitHub custom Pages workflow documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)

If Pages configuration checks or deployment fail, inspect the publishing source, repository variable, default branch and branch restrictions on the `github-pages` environment. Fix the reported cause and rerun the workflow. Do not suppress failures in place of a successful deployment. No additional personal access token is required by this workflow.

## Historical record: first online verification of 0.2.0

The following evidence belongs to the first successful publication on 2026-10-14. Later commits require their own Actions and Pages checks.

| Check | Evidence from the initial publication |
|---|---|
| Public repository | [market-data-toolkit-cpp](https://github.com/xinrui-dong/market-data-toolkit-cpp), confirmed accessible without signing in at that time |
| Verified commit | [d63f6d43063192c686cc360565a17de514eeacfc](https://github.com/xinrui-dong/market-data-toolkit-cpp/commit/d63f6d43063192c686cc360565a17de514eeacfc) |
| CI | [Push run 38650139416](https://github.com/xinrui-dong/market-data-toolkit-cpp/actions/runs/38650139416): all steps in both build checks and Pages deployment succeeded; publication was not skipped |
| Tests | 21 doctest cases and one CLI integration test: all 22 CTest entries passed |
| Benchmarks | The run's JSON artifact was checked: 18 workloads and 54 iteration records, with three repetitions per workload |
| Documentation | The [Pages home page](https://xinrui-dong.github.io/market-data-toolkit-cpp/) and [course API page](https://xinrui-dong.github.io/market-data-toolkit-cpp/namespacecourse.html) returned HTTP 200 without authentication at that time |

Before submitting, find the exact submission commit in [Actions](https://github.com/xinrui-dong/market-data-toolkit-cpp/actions), confirm that its checks and deployment completed successfully, and reopen the documentation links. Initial-publication records in this guide and the README do not replace verification of later commits.

## Pinned Actions versions

The workflow pins Actions to full commit SHAs, with adjacent comments naming their releases, as checked against the official repositories on 2026-10-14. Before updating an Action, verify its official release and compatibility, then update the SHA and release comment together.

| Action | Release used by this project | Official commit |
|---|---|---|
| `actions/checkout` | v7.0.1 | [3d3c42e5aac5ba805825da76410c181273ba90b1](https://github.com/actions/checkout/commit/3d3c42e5aac5ba805825da76410c181273ba90b1) |
| `actions/upload-artifact` | v7.0.2 | [cf430e030ddbb5b0abf93d22962f4752f3646cd9](https://github.com/actions/upload-artifact/commit/cf430e030ddbb5b0abf93d22962f4752f3646cd9) |
| `actions/upload-pages-artifact` | v5.0.0 | [fc324d3547104276b827a68afc52ff2a11cc49c9](https://github.com/actions/upload-pages-artifact/commit/fc324d3547104276b827a68afc52ff2a11cc49c9) |
| `actions/configure-pages` | v6.0.0 | [45bfe0192ca1faeb007ade9deae92b16b8254a0d](https://github.com/actions/configure-pages/commit/45bfe0192ca1faeb007ade9deae92b16b8254a0d) |
| `actions/deploy-pages` | v5.0.1 | [368f82528645a54fb793d4d04e342629a3f51346](https://github.com/actions/deploy-pages/commit/368f82528645a54fb793d4d04e342629a3f51346) |
