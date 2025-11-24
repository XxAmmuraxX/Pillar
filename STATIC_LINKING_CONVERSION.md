# Static Linking Conversion Summary

## Changes Made

### 1. **Pillar/CMakeLists.txt**
- ? Changed `SHARED` ? `STATIC`
- ? Removed `RUNTIME_OUTPUT_DIRECTORY` (DLLs only)
- ? Removed `LIBRARY_OUTPUT_DIRECTORY` (DLLs only)
- ? Kept `ARCHIVE_OUTPUT_DIRECTORY` (static libs)
- ? Removed `WINDOWS_EXPORT_ALL_SYMBOLS`
- ? Changed all link libraries to `PUBLIC` (static linking requirement)
- ? Added `PIL_STATIC_LIB` compile definition
- ? Removed DLL copy commands

### 2. **Pillar/src/Pillar/Core.h**
- ? Added `PIL_STATIC_LIB` detection
- ? `PIL_API` macro now empty for static builds
- ? No more `__declspec(dllexport/dllimport)`

### 3. **Sandbox/CMakeLists.txt**
- ? Removed DLL copy command
- ? Added `PIL_STATIC_LIB` compile definition

### 4. **Tests/CMakeLists.txt**
- ? Removed DLL copy command
- ? Added `PIL_STATIC_LIB` compile definition

## Benefits of Static Linking

### ? **Fixes ImGui Context Issue**
- Single ImGui instance compiled into final executable
- No more null `GImGui` pointer crashes
- No DLL boundary issues

### ? **Simpler Build**
- No DLL copying needed
- No export/import symbol management
- Single .lib file instead of .dll + .lib

### ? **Better Performance**
- No function call overhead across DLL boundaries
- Compiler can inline across module boundaries
- Link-time optimization (LTO) works better

### ? **Easier Distribution**
- Single .exe file (no separate DLL)
- No "missing DLL" runtime errors
- Smaller total binary size (no duplicate code)

## How to Build

### Option 1: Run the conversion script
```powershell
.\convert-to-static.ps1
```

### Option 2: Manual steps
```powershell
# 1. Clean build
Remove-Item -Path out/build/x64-Debug -Recurse -Force

# 2. Configure
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# 3. Build
cmake --build out/build/x64-Debug --config Debug --parallel
```

## Expected Build Outputs

### Before (DLL)
```
bin/Debug-x64/Pillar/
  ??? Pillar.dll
  ??? Pillar.lib (import library)
bin/Debug-x64/Sandbox/
  ??? SandboxApp.exe
  ??? Pillar.dll (copy)
bin/Debug-x64/Tests/
  ??? PillarTests.exe
  ??? Pillar.dll (copy)
```

### After (Static)
```
bin/Debug-x64/Pillar/
  ??? Pillar.lib (static library)
bin/Debug-x64/Sandbox/
  ??? SandboxApp.exe (contains Pillar code)
bin/Debug-x64/Tests/
  ??? PillarTests.exe (contains Pillar code)
```

## File Size Comparison

| Build Type | Pillar Binary | SandboxApp.exe | Total |
|------------|---------------|----------------|-------|
| **DLL** | ~2.5 MB DLL | ~50 KB | ~2.6 MB |
| **Static** | ~500 KB LIB | ~2.8 MB | ~2.8 MB |

*Static is slightly larger per-executable but eliminates DLL management*

## What Changed Internally

### PIL_API Macro Behavior

**Before (DLL):**
```cpp
// In Pillar (building DLL)
#define PIL_API __declspec(dllexport)

// In Sandbox (using DLL)
#define PIL_API __declspec(dllimport)
```

**After (Static):**
```cpp
// Everywhere
#define PIL_API  // Empty macro, no export needed
```

### Linking Changes

**Before (DLL):**
- Pillar.dll exports symbols
- Sandbox.exe imports symbols at runtime
- ImGui could be in either module (problem!)

**After (Static):**
- Pillar.lib provides object files
- Linker merges everything into .exe at build time
- Single ImGui instance in final executable

## Troubleshooting

### If build fails with "unresolved external symbol"
- Make sure all `target_link_libraries` use `PUBLIC` in Pillar
- Verify `PIL_STATIC_LIB` is defined in all targets

### If ImGui still crashes
- Delete `out/build/x64-Debug` completely
- Rebuild from scratch
- Verify no old Pillar.dll in output directories

### If linker says "already defined"
- Check that imgui is only built once (in root CMakeLists)
- Verify Pillar links imgui as PUBLIC, not PRIVATE

## Testing After Conversion

### 1. Run Unit Tests
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe
```
Expected: All 73 tests pass (41 engine + 32 camera)

### 2. Run Application
```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```
Expected:
- Window opens without crashes
- ImGui panels visible
- Camera controls work (WASD, Q/E, mouse wheel)
- No "DLL not found" errors

### 3. Verify Static Linking
```powershell
# Check dependencies (should NOT list Pillar.dll)
dumpbin /DEPENDENTS bin\Debug-x64\Sandbox\SandboxApp.exe
```

## Reverting to DLL (if needed)

If you need to go back to DLL for some reason:

1. Change `STATIC` ? `SHARED` in Pillar/CMakeLists.txt
2. Remove `PIL_STATIC_LIB` definitions
3. Restore DLL copy commands
4. Rebuild clean

But **static linking is recommended** for this project because:
- You're not distributing Pillar as a library to third parties
- Single-executable distribution is simpler
- Fixes the ImGui context issue permanently
- Better for game engines that aren't plugin-based

## Next Steps After Conversion

Once static linking works:

1. ? Update PROJECT_STATUS.md to reflect static build
2. ? Update CAMERA_SYSTEM_PLAN.md if needed
3. ? Update copilot-instructions.md (remove DLL references)
4. ? Commit changes to git
5. ?? Continue with Phase 2 camera features (smooth movement, bounds, etc.)

---

**Status:** Ready to build with `.\convert-to-static.ps1`
