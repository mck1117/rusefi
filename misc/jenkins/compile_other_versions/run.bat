set script_name=run.bat
echo Entering %script_name%
echo "RUSEFI_BUILD_FTP_USER=%RUSEFI_BUILD_FTP_USER%"

pwd

call misc\jenkins\build_java_console.bat
if not exist java_console_binary/rusefi_console.jar exit -1

call misc\jenkins\build_simulator.bat
if not exist simulator/build/rusefi_simulator.exe exit -1

call misc\jenkins\compile_other_versions\compile_and_upload.bat kinetis kinetis
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1

call misc\jenkins\compile_other_versions\compile_and_upload.bat frankenso frankenso_na6
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

set f7_console_setting=firmware\config\boards\nucleo_f767\rusefi_console_properties.xml

rem                     folter_name configuration_name [optional .ini file name]

call misc\jenkins\compile_other_versions\compile_and_upload.bat proteus     proteus-f4 rusefi_proteus.ini
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

call misc\jenkins\compile_other_versions\compile_and_upload.bat proteus     proteus-f7 rusefi_proteus.ini
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

call misc\jenkins\compile_other_versions\compile_and_upload.bat microrusefi mre-f4 rusefi_microrusefi.ini
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

call misc\jenkins\compile_other_versions\compile_and_upload.bat microrusefi mre-f4-test rusefi_microrusefi.ini
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

call misc\jenkins\compile_other_versions\compile_and_upload.bat microrusefi mre-f7 rusefi_microrusefi.ini %f7_console_setting%
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

call misc\jenkins\compile_other_versions\compile_and_upload.bat microrusefi mre-f7-test rusefi_microrusefi.ini %f7_console_setting%
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

call misc\jenkins\compile_other_versions\compile_and_upload.bat nucleo_f746 stm32f746_nucleo no %f7_console_setting%
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd


call misc\jenkins\compile_other_versions\compile_and_upload.bat nucleo_f767 stm32f767_nucleo no %f7_console_setting%
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd


call misc\jenkins\compile_other_versions\compile_and_upload.bat nucleo_f767 stm32f767_osc no %f7_console_setting%
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd


call misc\jenkins\compile_other_versions\compile_and_upload.bat prometheus prometheus_405
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd


call misc\jenkins\compile_other_versions\compile_and_upload.bat prometheus prometheus_469
IF NOT ERRORLEVEL 0 echo ERROR invoking compile_and_upload.bat
IF NOT ERRORLEVEL 0 EXIT /B 1
pwd

