@echo off
set "outfile=combined_code.txt"

:: Clean up old output file if it exists
if exist "%outfile%" del "%outfile%"

:: Loop through the specific file extensions
for %%f in (*.cu *.h *.cpp) do (
    echo Processing %%f...
    echo. >> "%outfile%"
    echo --- START OF FILE: %%f --- >> "%outfile%"
    type "%%f" >> "%outfile%"
    echo. >> "%outfile%"
    echo --- END OF FILE: %%f --- >> "%outfile%"
    echo. >> "%outfile%"
)

echo.
echo Success. Combined code is in %outfile%.
pause