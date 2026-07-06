# GitHub apply instructions

This tree is ready to push into `localhaos/chrlauncher`.

The GitHub connector reported write failure for this session:

```text
403 Resource not accessible by integration
```

Apply locally from a clean clone:

```bat
git clone https://github.com/localhaos/chrlauncher.git chrlauncher
cd chrlauncher
git checkout -b fix/split-config-full-project

git apply ..\chrlauncher_full_project_github_ready.patch

git status
git add .
git commit -m "Fix split config loader and Windows build"
git push -u origin fix/split-config-full-project
```

Then open a pull request from `fix/split-config-full-project` into `master`.

Runtime/build validation:

```bat
build_vc.bat
```

Expected local outputs:

```text
bin\chrlauncher.x86.exe
bin\chrlauncher.x64.exe
```

GitHub Actions validation:

- workflow: `.github/workflows/windows-build.yml`
- matrix: `Win32`, `x64`
- artifacts: `chrlauncher-x86`, `chrlauncher-x64`
