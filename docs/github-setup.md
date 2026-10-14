# GitHub Actions 与文档发布

这个目录提供金融历史行情 CSV 检查、收益率与两种 SMA 算法的 C++ 项目。已有 workflow 文件并不表示它已在 GitHub 运行成功；本地构建成功也不等于线上 CI 或 Pages 验证成功。

## 上传时选择正确的仓库根目录

**新 GitHub 仓库应以本地 `cpp-course-template/` 的内容为根。** GitHub 仓库首页应直接看到 `CMakeLists.txt`、`README.md`、`AGENTS.md`、`src/`、`tests/`、`benchmarks/` 和 `.github/`。

不要把包含课件、作业 PDF 和本机环境文件的整个课程文件夹上传。也不要再给源码套一层 `cpp-course-template/`：GitHub 只识别仓库根目录下的 `.github/workflows/`，本 workflow 的 CMake 命令也从仓库根目录运行。

决定正式仓库名称、准备好账号和发布设置后，再执行下面的步骤。本文件没有创建远程仓库或推送代码。

1. 在 GitHub 创建公开仓库，名称使用最终确定的项目名。
2. 将本模板的源文件放到该仓库根目录，保留隐藏的 `.github/` 和 `.gitignore`，以及已随模板提供的第三方源码包和许可说明。
3. 不上传 `build/`、生成的 HTML、本机环境目录或凭据。依赖源码包按模板现有结构保留，保证新克隆也能构建。
4. 推送后打开仓库的 **Actions**，检查 `Build, test, benchmark and docs` 这次运行的每个步骤。
5. 后续修改功能时同步更新测试、benchmark、README 和 API 文档，再重新运行全部验证。

## CI 自动运行什么

`push`、`pull_request` 和手动 `workflow_dispatch` 都触发同一条 Ubuntu 流程：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --no-tests=error
cmake --build build --target run_benchmarks
cmake --build build --target docs
```

CI 安装 CMake、G++、Make 和 Doxygen。单元测试和 benchmark 所需的第三方源码已随模板提供，CI 不再通过 apt 安装这两个库。实际工具版本在 Actions 日志中显示。

benchmark 的实测结果打印在 **Run benchmarks** 日志中，并写入 `build/benchmark.json`；Doxygen HTML 输出到 `build/docs/html/`。成功运行后，Actions 页面提供 `benchmark-results` 和 `doxygen-html` 两个 artifact，当前保留期设置为 14 天。artifact 是供下载检查的运行产物，**上传 artifact 不等于已发布公开文档网站**。

流程没有用 `continue-on-error` 忽略编译、测试或测速错误。`--no-tests=error` 防止“没有发现任何测试”被当作测试成功。同一 ref 的新运行会取消旧运行，其他分支的运行继续保留；最终验收需等待准备提交的那次运行完整成功。

## 启用 GitHub Pages

模板使用仓库变量 **`ENABLE_PAGES`** 作为明确的发布开关。未设置或未设为 `true` 时，构建、测试、测速、Doxygen 生成和普通 artifact 上传仍运行，Pages 上传与部署会显示跳过。

**Pages 部署跳过时，作业第 5 项“自动发布文档”尚未完成，不能据此声称第 5 项已满足满分要求。**

在正式公开仓库准备好之后：

1. 打开仓库 **Settings → Pages**。
2. 在 **Build and deployment → Source** 选择 **GitHub Actions**。已有本模板的 workflow，无需再添加另一份 Pages 模板。[GitHub 官方发布源说明](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)
3. 打开 **Settings → Secrets and variables → Actions → Variables**，选择 **New repository variable**。
4. 名称填写 `ENABLE_PAGES`，值填写小写的 `true`，保存。这是普通仓库变量，不是 Secret。[GitHub 官方变量说明](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/use-variables)
5. 回到 **Actions**，打开本 workflow，使用 **Run workflow**，选择仓库默认分支运行；也可再次向默认分支推送一项实际修改。要重跑完整流程，不能只重跑此前跳过的部署 job。
6. 等待 `CMake, CTest, benchmark and Doxygen` 与 `Publish Doxygen to GitHub Pages` 都成功。
7. 打开部署输出的 `page_url`，检查首页和至少一个示例函数页面。
8. 将实际访问成功的地址写到 README 的文档链接位置。地址常见格式为 `https://<GitHub用户名>.github.io/<仓库名>/`；这只是格式示意，须以部署返回的真实地址为准。
9. 推送 README 或文档的实际更新，确认默认分支的新提交会自动重新部署。

Pages 只在开关启用、且运行来自**默认分支的 push 或默认分支的手动触发**时部署。其他分支及 pull request 仍运行完整构建检查并保存 HTML artifact，但不会覆盖正式站点。

部署 job 依赖构建检查成功，使用 `github-pages` 环境和 `pages: write`、`id-token: write` 权限；构建 job 只有源码读取权限。这与 GitHub 官方的分离构建/部署方案一致。[GitHub 官方自定义 Pages workflow 说明](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)

若 Pages 配置检查或部署失败，先检查 Source、仓库变量、默认分支与 `github-pages` 环境的分支限制；按报错修复后重跑。不要用忽略错误代替真正部署成功。无需为本模板额外保存个人访问令牌。

## 线上验收记录

正式提交前，需要记录实际结果；以下值目前留空，不能预填成“已通过”。

| 检查项 | 待填写的实际证据 |
|---|---|
| 公开仓库 | 仓库真实链接；未登录可访问 |
| 最终代码 | 准备提交的 commit ID |
| CI | 对应该 commit 的 Actions 运行链接及成功状态 |
| 单元测试 | 实际执行的测试数、通过结果 |
| benchmark | 日志中的输入规模、耗时和单位；JSON artifact |
| 文档 | 该 commit 的 Pages 部署成功及可访问地址 |
| README | 文档链接已替换为真实地址 |

只有 GitHub 上实际执行成功的日志才能证明线上 CI；只有成功部署并可访问的页面才能证明自动文档发布。当前准备好的模板与本地运行结果，不代替这两类证据。

## Actions 版本依据

2026 年 10 月 14 日核对 GitHub 官方仓库后，workflow 将 action 固定到完整提交 SHA，并在旁边注释对应 release。更新时应先核对官方 release 和兼容性，再同时更新 SHA 与注释。

| Action | 本模板采用的 release | 官方提交 |
|---|---|---|
| `actions/checkout` | v7.0.1 | [3d3c42e5aac5ba805825da76410c181273ba90b1](https://github.com/actions/checkout/commit/3d3c42e5aac5ba805825da76410c181273ba90b1) |
| `actions/upload-artifact` | v7.0.2 | [cf430e030ddbb5b0abf93d22962f4752f3646cd9](https://github.com/actions/upload-artifact/commit/cf430e030ddbb5b0abf93d22962f4752f3646cd9) |
| `actions/upload-pages-artifact` | v5.0.0 | [fc324d3547104276b827a68afc52ff2a11cc49c9](https://github.com/actions/upload-pages-artifact/commit/fc324d3547104276b827a68afc52ff2a11cc49c9) |
| `actions/configure-pages` | v6.0.0 | [45bfe0192ca1faeb007ade9deae92b16b8254a0d](https://github.com/actions/configure-pages/commit/45bfe0192ca1faeb007ade9deae92b16b8254a0d) |
| `actions/deploy-pages` | v5.0.1 | [368f82528645a54fb793d4d04e342629a3f51346](https://github.com/actions/deploy-pages/commit/368f82528645a54fb793d4d04e342629a3f51346) |
