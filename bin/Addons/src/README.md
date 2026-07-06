# Addons native source tree

Native addon code that must be compiled into `chrlauncher` should live under:

```text
bin\Addons\src\<addon_name>\
```

Runtime addon files should stay under:

```text
bin\Addons\<addon_name>\
```

Example:

```text
bin\Addons\optimizer_guard\optimizer_guard.ini
bin\Addons\optimizer_guard\apply-optimizer-guard.bat
bin\Addons\src\optimizer_guard\optimizer_guard_bootstrap.c
```

`Directory.Build.props` is the compile bridge. It may include individual native addon sources, for example:

```xml
<ClCompile Include="$(MSBuildThisFileDirectory)bin\Addons\src\optimizer_guard\optimizer_guard_bootstrap.c" />
```

Rules:

- keep addon runtime/config/docs separate from compiled C/C++ source;
- use one folder per addon name;
- native addon bootstrap files should be small and self-contained;
- avoid hard dependencies between addons unless explicitly documented;
- preserve startup order with CRT section names when order matters.
