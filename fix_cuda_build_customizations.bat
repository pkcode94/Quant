@echo off
:: ============================================================
:: fix_cuda_build_customizations.bat
::
:: Copies CUDA 13.1 Build Customization files (including the
:: MSBuild task DLL) into BOTH v170 and v180 BuildCustomizations
:: folders.  The CUDA installer only partially populated v180
:: and skipped v170 entirely.
::
:: RIGHT-CLICK this file and choose "Run as administrator".
:: ============================================================

set SRC=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1\extras\visual_studio_integration\MSBuildExtensions
set V170=C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Microsoft\VC\v170\BuildCustomizations
set V180=C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Microsoft\VC\v180\BuildCustomizations

echo Copying CUDA 13.1 build customizations from toolkit source...
echo   FROM: %SRC%
echo   TO:   %V170%
echo   TO:   %V180%
echo.

for %%D in ("%V170%" "%V180%") do (
    copy "%SRC%\CUDA 13.1.props"                %%D\ /Y
    copy "%SRC%\CUDA 13.1.targets"              %%D\ /Y
    copy "%SRC%\CUDA 13.1.Version.props"        %%D\ /Y
    copy "%SRC%\CUDA 13.1.xml"                  %%D\ /Y
    copy "%SRC%\Nvda.Build.CudaTasks.v13.1.dll" %%D\ /Y
    echo.
)

echo.
if errorlevel 1 (
    echo FAILED - make sure you ran this as Administrator.
) else (
    echo SUCCESS - restart Visual Studio and rebuild.
)
pause
